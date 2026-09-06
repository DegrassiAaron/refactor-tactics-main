# «Roadmap Schematiche: Editor Execution» — spec panel

> `CURRENT` · **Stato**: revisione chiusa. Il sorgente è **recensito, non applicato**: nessuna roadmap
> nuova creata · **Data**: 2026-09-06
> **HEAD della misura**: `2a08f4c2`, **rimisurato su `2eb32f34`**, **riallineato a `28c60b9e`** —
> `origin/main` si è mosso **tre volte** durante la run; §3 `R8` dice cosa è cambiato e cosa no
> (worktree `docs/schematiche-editor-execution-spec-panel`)
> **Oggetto**: il work order esterno *«RefactorTactics — Roadmap Schematiche: Editor Execution»*, fornito
> in chat **senza sha né data**, che chiede di eseguire e validare in Editor quindici schematiche `S01…S15`
> attraverso otto fasi `ED0…ED8`, e rimanda a una `schematiche-code-only-roadmap-2026-09-05.md`.
> **Panel**: Wiegers (lead) · Fowler · Nygard · Crispin · Adzic · Cockburn
> **Modo**: critique · **Focus**: requirements, architecture, testing
> ⚠️ **Il documento non è nel repository e non entra**: è una consegna effimera. Questo referto è l'unico
> posto in cui il suo contenuto resta citabile — [`AGENTS.md`](../../../AGENTS.md) §8.
> 🔑 **Nessun numero qui è ricordato.** Ogni conteggio porta il comando che l'ha prodotto, in §3.

---

## 1. Il verdetto in una riga

Il work order è **scritto bene e arriva settimo**: la sua tesi centrale — *l'Editor esegue e mostra, non
decide* — è corretta e il repository la condivide, ma è **già scritta** in `CLAUDE.md` §5 e in
[`AGENTS.md`](../../../AGENTS.md) §8, che il ciclo precedente di questi stessi kit ha prodotto. Nel
frattempo la roadmap che chiede di creare **esiste ed è `CURRENT`**, l'oggetto che dovrebbe eseguire — le
quindici schematiche — **non è nel repository**, e il gate che elegge a primo passo **dà il verdetto
sbagliato sul gioco spedito**.

| | Rilievo | Gravità |
|---|---|---|
| **R1** | è il **settimo giro dello stesso contratto in 72 ore**, e non nomina nessuno dei referti che lo hanno già revisionato — né [`AGENTS.md`](../../../AGENTS.md) §8, che quel ciclo ha prodotto e che dichiara *«non entra il preambolo di processo»* | 🔴 |
| **R2** | **la Roadmap Editor esiste già**: [`roadmap-esecuzione-pie-2026-09-03.md`](roadmap-esecuzione-pie-2026-09-03.md) è `CURRENT` e dichiara testualmente la stessa tripartizione — *«il registro degli esiti resta `test-manuali-pie.md`; quale voce e quando resta `editor-sessions.yaml`; questo file dice in che ordine e cosa manca»* | 🔴 |
| **R3** | **l'oggetto non esiste**: zero file `*schemati*` e zero `*code-only*` in entrambi i checkout. §2 rimanda `BLOCKED-CODE` a un documento che nessuno può aprire | 🔴 |
| **R4** | **il gate `S05` sbaglia sul gioco spedito**: `Action.Sprint` è profilo di `Move` per il canone e risolve **pre-Blast** nel codice — arretrato dichiarato `#641`/`D-116`. Il kit non dice mai **contro cosa** si misura una schematica | 🔴 |
| **R5** | il vocabolario di stato **conferisce alle schematiche l'autorità che §2 nega all'Editor**: `CANONICAL` è per definizione *«la fonte che prevale in caso di conflitto»*, e *«il posto dove si cambia una regola»* | 🔴 |
| **R6** | §3 **riclassifica l'oracolo dove `CLAUDE.md` §5 vieta di riclassificarlo**; le classi `A/B/C/D` esistono, sono misurate due volte, e la **D** — che al kit manca — è proprio quella di `S14` e `S15` | 🟠 |
| **R7** | degli **owner nominati** dal kit, **quattro su cinque sono chiusi** — cioè **sei schematiche su quindici** poggiano su un owner chiuso — e `MOV-4` è dichiarata aperta mentre `D-325` la chiude dal 2026-09-04 | 🟠 |
| **R8** | `S01…S15` è un **terzo spazio di identificatori** accanto alle sedute `U*` e alle voci `PIE-*`, senza una mappa fra i tre e senza dire quale voce del registro una `S` aggiorni | 🟠 |
| **R9** | §5 definisce un bundle di evidenza **senza sede**, e il manifest non porta né lo stato del working tree né lo sha del binario | 🔵 |
| **R10** | `BLOCKED-MCP` è **più debole** del contratto già scritto: `CLAUDE.md` §5 prescrive di ripetere la query fuori da PIE prima di dichiarare assente una capability | 🔵 |

**Cosa si salva, e vale la run**: **tre** contributi che nessun owner possiede oggi — §5 — più **due
difetti trovati per strada nel repository**, corretti in questo stesso pass — §6.

**Raccomandazione del panel**: **non creare la roadmap.** Portare i tre contributi a
[`roadmap-esecuzione-pie-2026-09-03.md`](roadmap-esecuzione-pie-2026-09-03.md) e al registro, che li
possiedono già entrambi.

---

## 2. Perché la tesi giusta non basta

**COCKBURN**: *«Chi è l'attore, e cosa apre?»* — La §2 del kit è la parte migliore: *l'Editor può aprire,
eseguire, osservare; non può correggere un resolver, scegliere una decisione aperta, o inventare una
regola in Blueprint.* È vera, è netta, e chiude il modo tipico in cui una sessione visuale diventa una
decisione presa per inerzia.

⛔ **Ed è esattamente ciò che [`AGENTS.md`](../../../AGENTS.md) §8 dichiara di non accettare da un kit**:

> *«Non entra il **preambolo di processo**. Ricognizione, anti-duplicazione, priorità, label, milestone e
> formato del report sono già scritti qui e in `CLAUDE.md`. Un kit che li riscrive non li sostituisce, e
> non li emenda.»*

🔑 **La differenza non è formale.** Le §2, §3, §4 e §5 del kit riscrivono quattro contratti esistenti, e
in due punti li riscrivono **più deboli** — `R6` e `R10`. Un contratto riscritto peggio non è una
conferma: è una biforcazione, e il lettore che ne trova due versioni sceglie quella che ha davanti.

⚠️ **Questa nota corregge la prima lettura del panel**, che aveva assegnato a §2 il voto più alto del kit.
Il testo resta buono; la sede è occupata.

---

## 3. I rilievi, con la misura che li produce

Ogni comando è stato eseguito nel worktree su `2a08f4c2`, e rieseguito su `2eb32f34` dopo che
`origin/main` si è mosso. ⚠️ **Non «reggono tutti»**, che è ciò che la prima stesura di questa riga
affermava dopo aver ricontrollato due totali: i totali reggono, e una grandezza citata da `R2` — le voci
schedulate contro le orfane — è **proprio** quella che l'evento intercorso ha spostato. Dettaglio in `R8`.

### R1 — Il settimo giro

```bash
ls docs/roadmap/plans/*2026-09-0[3-6]*.md | wc -l   # -> 27 referti in quattro giorni, questo compreso
```

⚠️ **Il comando conta il volume, non seleziona la tabella**: dei ventisette, i quattro qui sotto sono
quelli **letti** e pertinenti — tre revisionano la stessa forma (*«due roadmap, una code, una Editor, più
una matrice»*), il quarto è quello che ne ha ricavato il contratto. La selezione è a mano e si dichiara,
perché nessun `grep` la produce: `gov4-…` non contiene né `dual-roadmap` né `spec-panel` nel nome.

| Data | Referto | Esito |
|---|---|---|
| 09-04 | [`cloud-dual-roadmap-spec-panel-2026-09-04.md`](cloud-dual-roadmap-spec-panel-2026-09-04.md) | ~15% salvato, il resto consegnato ad `AGENTS.md` §9, `CLAUDE.md` §5 e `D-330` |
| 09-05 | [`dual-roadmap-code-editor-validation-triage-2026-09-05.md`](dual-roadmap-code-editor-validation-triage-2026-09-05.md) | *«secondo giro dello stesso contratto in 24 ore»*, nessuna roadmap creata |
| 09-05 | [`primary-fix-dual-roadmap-spec-panel-2026-09-05.md`](primary-fix-dual-roadmap-spec-panel-2026-09-05.md) | *«sesto giro in 48 ore»*, nessuna roadmap creata |
| 09-05 | [`gov4-contratto-work-order-esterni-2026-09-05.md`](gov4-contratto-work-order-esterni-2026-09-05.md) | ha prodotto il **contratto** oggi in `AGENTS.md` §8 |

🔑 **Il kit non nomina nessuno dei quattro.** E il quarto è quello che regola i primi tre.

⚠️ **Da dove viene il «settimo».** Non è un conteggio indipendente: è la misura di
[`primary-fix`](primary-fix-dual-roadmap-spec-panel-2026-09-05.md) — *«sesto giro dello stesso contratto
in 48 ore»*, scritta il 2026-09-05 — **più questo**. Chi volesse contestarlo deve contestare quella, non
questa riga.

### R2 — La roadmap esiste

```bash
head -10 docs/roadmap/plans/roadmap-esecuzione-pie-2026-09-03.md
```

Il file è `CURRENT`, si dichiara owner *«fino a quando le sedute che elenca sono aperte»*, e possiede già
la ripartizione che il kit §1 ridisegna. Porta anche la misura che il kit non ha: il collo di bottiglia
dell'esecuzione Editor **non è la mancanza di schematiche**, sono le voci aperte che nessuna seduta
convoca — `38`, poi rimisurate a `29` orfane — e per la seduta più ricca (`U42`, ventuno voci in undici
Play) le issue da implementare sono **zero**.

∴ Una roadmap Editor nuova non sblocca nulla che sia bloccato: aggiunge un secondo piano sopra un piano
che nessuno esegue.

### R3 — L'oggetto non è nel repository

```bash
find . -path ./.git -prune -o \( -iname "*schemati*" -o -iname "*code-only*" \) -print \
  | grep -v spec-panel                                    # -> 0  (misurato su 2a08f4c2)
ls D:/Repositories/refactor-tactics-main/docs/src/        # -> non esiste
```

⚠️ **Il filtro `grep -v spec-panel` non c'era nella prima stesura, e serve dal commit che scrive questo
referto**: `schematiche-editor-execution-spec-panel-2026-09-06.md` matcha `*schemati*`. Chi rieseguisse il
comando nudo per controllare `R3` troverebbe **un** risultato — questo file — e dovrebbe dedurre da sé che
non è una schematica.

Zero in entrambi i checkout. `S01…S15` e la roadmap code-only sono materiale esterno. ⚠️ Sono quindi
**input da rimisurare, non canone** — e il kit li tratta come artefatti con owner assegnato.

### R4 — Il gate `S05` e lo Sprint 🔴

**ADZIC**: *«Datemi l'esempio in cui la regola è falsa.»* — È nel gioco spedito.

Il falsificatore del kit §9: *«Il Move normale è dopo Blast. Una schematica che mostra il contrario
fallisce anche se appare più intuitiva.»* Il canone concorda —
[`spec-sequenza-turno.md`](../../gameplay/spec-sequenza-turno.md) §1. Il codice no:

| Fonte | `Action.Sprint` |
|---|---|
| Canone `D-015` / `D-116` | profilo di **`Move`** → post-Blast |
| `RTCatalogLibrary.cpp:1065` | `ERTResolutionPhase::FastMovement` |
| `RTCatalogLibrary.cpp:177` | `FastMovement → ERTMatchPhase::Dash` — **pre-Blast** |
| Test | `Actions.SprintIsAMoveProfileResolvedPreBlast` — verde |

Il commento del test lo dichiara: *«il verde qui sotto misura quanto il codice è indietro, non che abbia
ragione»*. Migrazione **E38 (v0.2)**, issue `#641`.

🔴 **Conseguenza operativa.** Un operatore che pianifica uno Sprint in PIE vede il movimento risolversi
*prima* del Blast. Col falsificatore così com'è ha due uscite, **entrambe sbagliate**: dichiarare `FAIL`
una schematica canonicamente corretta, oppure allineare la schematica all'osservazione — cioè ratificare
un arretrato per inerzia, che è ciò che il kit §16 vieta.

🔑 **La causa a monte**: il kit non dice mai **contro cosa** si giudica una schematica. Canone o
comportamento osservato? Le due cose divergono in almeno un punto noto, e le schematiche sono quindici.

### R5 — Il vocabolario capovolge l'intento

[`docs/README.md`](../../README.md) §*Tag* definisce:

| Tag | Significato *(col. 2)* | Si aggiorna? *(col. 3)* |
|---|---|---|
| `CANONICAL` | **«Decide: è la fonte che prevale in caso di conflitto»** | «Sì — **è il posto dove si cambia una regola**» |
| `CURRENT` | «Descrive com'è il progetto **oggi**, subordinato al canone» | «Sì» |
| `OPEN` | «Aspetta una decisione umana» | «**Vive in `OPEN_DECISIONS.md`**» |

⚠️ **Le colonne sono tenute separate di proposito**: la sorgente è una tabella a tre colonne, e unirle in
una citazione sola produrrebbe una stringa che non esiste nel file — chi la cercasse per verificare il
rilievo non troverebbe nulla.

Il kit §6 marca `CANONICAL` sei schematiche. Per la definizione del tag, una schematica `CANONICAL`
**prevale su [`spec-sequenza-turno.md`](../../gameplay/spec-sequenza-turno.md)** — cioè esercita il potere
che §2 nega all'Editor, concesso dall'etichetta. Il tag di una schematica è `CURRENT`. E `OPEN` non è
un'etichetta di documento; *«OPEN parziale»* (`S02`, `S03`, `S13`) fonde due assi distinti — la maturità
del documento e lo stato della decisione che lo blocca.

### R6 — L'oracolo è già ripartito, e la classe che manca è quella che serve

`CLAUDE.md` §5, testualmente: *«la ripartizione è in `scenario-map.md`, e non si riclassifica qui»*.

[`scenario-map.md`](../../technical/tooling/scenario-map.md) §2 ha quattro classi, misurate due volte con
metodi indipendenti:

| Classe | Oracolo |
|---|---|
| **A** | la macchina esegue e giudica |
| **B** | la macchina esegue, **l'umano giudica** |
| **C** | solo umano — mouse, editor, giudizio, cronometro |
| **D** | **dichiarato, non eseguibile: il sistema non esiste** |

`HUMAN-SIGNOFF` è la classe **B** con un nome nuovo. ⚠️ E la classe **D** è quella che al kit manca e che
gli servirebbe: `S14` (che §13 impone resti `PROPOSED`) e la parte di `S15` che aspetta `M10.3` non sono
*bloccate*, sono **non eseguibili perché il sistema non c'è** — e il kit le mette in `BLOCKED-*` insieme a
cose ferme per ragioni diverse.

### R7 — Gli owner, misurati

```bash
gh issue view <n> --json state,closedAt ; grep -n 'MOV-4' docs/OPEN_DECISIONS.md
```

| S | Owner dichiarato | Stato nel kit | Misurato |
|---|---|---|---|
| `S03`, `S09` | `MOV-4` | «OPEN parziale» / «misto» | 🔴 **chiusa il 2026-09-04 — `D-325`**, barrata in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| `S01`, `S05` | `#2277` | CANONICAL | CLOSED 2026-09-04 |
| `S08` | `#1918` | «misto» | CLOSED 2026-09-01 — e `D-312` ha deciso: `Deflect` prima di `Guard` |
| `S15` | `#589` | «CANONICAL come vincolo» | CLOSED 2026-09-04, e ha **già consegnato** `Privacy.ServerOnlyTypesAreNotReplicated` |
| `S02`, `S13` | `#2148` | «OPEN parziale» | ✅ **OPEN** — l'unico corretto |

Due conseguenze. *(a)* Il kit §10 istruisce `BLOCKED-DECISION` per i casi *«ancora posseduti da #2148»*:
corretto per `#2148`, ma chi applicasse lo stesso riflesso a `MOV-4` — che §6 presenta come aperta —
fermerebbe il lavoro su una decisione presa, e
[`D-325`](../../decisions/RT_PDR_00_Decision_Log.md) dice pure **cosa**: `FRTActionDef::Priority` resta.
*(b)* `S15` omette il gate strutturale esistente. ✅ **La frase del kit §14 resta giusta e va tenuta** —
una simulazione locale non prova la privacy di rete, e il canary `M10.3` va eseguito **packaged** — ma
*«non c'è nulla»* è falso, ed è già l'errore che [`CONTEXT_INDEX.md`](../../CONTEXT_INDEX.md) ha dovuto
correggere una volta.

### R8 — Il terzo spazio di ID

```bash
grep -oE "^  - id: U[0-9]+" docs/roadmap/editor-sessions.yaml | sort -u | wc -l   # -> 48
grep -c '^| \*\*PIE-' docs/technical/test-manuali-pie.md                          # -> 226  (comando canonico)
```

🔴 **La seconda riga è stata corretta durante la review di questo stesso referto, e l'errore vale più del
numero.** La prima stesura contava le voci con `grep -oE "PIE-[A-Z0-9-]+" | sort -u | wc -l` e pubblicava
**228**. È il metodo che
[`roadmap-esecuzione-pie-2026-09-03.md`](roadmap-esecuzione-pie-2026-09-03.md) §*Metodo* dichiara
sbagliato **per nome** — *«stesso criterio del comando canonico del registro: le **righe di tabella**, non
un `grep` sugli ID»* — e con la ragione già misurata: la classe `[A-Z0-9-]` perde le voci con una
minuscola (`PIE-AS4a`, `PIE-BU2c`, `PIE-HEXPLAY-3b`), collassandole sulla sorella, e in cambio conta nove
**prefissi di famiglia** che non sono voci (`PIE-VIS-`, `PIE-TD-`, `PIE-V01-`…). ⚠️ **I due errori si
compensano quasi**, ed è ciò che rende `228` credibile: due sbagli che si annullano somigliano a una
misura. 🔑 In un documento che si apre dichiarando *«nessun numero qui è ricordato»* — e che contesta al
kit di non misurare — questo è il difetto peggiore possibile, e resta scritto qui invece di sparire nel
diff.

`S01…S15` si aggiunge a **48** sedute e **226** voci, senza una mappa fra i tre. Il kit §16 chiede che
*«il registro PIE venga aggiornato solo dopo verifiche realmente eseguite»*, ma nessuna `S` dichiara
**quale voce** aggiorna. Una `S` eseguita senza quella riga non tocca niente, e ricade nella condizione
che il registro descrive già per `PIE-SCEN-COMPOSER`: *«nel registro e fuori da ogni sequenza»*.

🔄 **Rimisurato su `2eb32f34`**, dove `origin/main` si è spostato mentre questo referto era in scrittura —
e il commit che è arrivato è **il precedente che questo rilievo chiede**. La PR `#2520` aggancia
`PIE-V01-PREPWINDOW` alla seduta `U23`: aggiunge la voce sotto `verifies:` nei dati della seduta — **lista
a blocchi**, non la forma inline `verifies: [...]`, e la distinzione conta perché
[`roadmap-esecuzione-pie`](roadmap-esecuzione-pie-2026-09-03.md) registra che leggerne una sola *«perde
metà delle sedute»* — annota `📌 Seduta: U23` sulla voce del registro, e dichiara la classe: *«nessuno
scenario può sostituirla… classe **C** in `scenario-map.md` §5»*. 🔑 **È esattamente il legame che `R8`
dice mancare e la classificazione che `R6` dice esistere**, fatti a mano su una voce. Il difetto non è che
non si sappia fare: è che non è un campo, e va rifatto a mano ogni volta.

⚠️ **E un conteggio del kit-report È cambiato, contro quanto la prima stesura di questa riga affermava.**
Restano `48` sedute e `226` voci — un aggancio non conia identificatori — ma *«voci schedulate / orfane»*
è **proprio** la grandezza che un aggancio sposta: `73/29` diventa `74/28`. La cifra citata in `R2` viene
da [`roadmap-esecuzione-pie`](roadmap-esecuzione-pie-2026-09-03.md) e **non è stata rimisurata qui**: vale
alla sua data. 🔑 La lezione è la stessa di `R8`: *«i conteggi reggono»* verificato su due totali che
l'evento non poteva muovere non è una verifica dei conteggi.

### R9 / R10 — I due minori

- Il bundle `Sxx/` non ha un path, e nessun owner lo possiede. Il manifest porta `Commit` ma non
  `WorkingTree` né lo **sha del binario**: in questo repository sorgenti e DLL possono raccontare due
  branch diversi, e un esito PIE su albero sporco si decide per sottosistema toccato dal diff.
- `BLOCKED-MCP` omette la contro-misura che `CLAUDE.md` §5 prescrive in due frasi distinte, a quattro
  righe di distanza: *«Una risposta vuota non è un no»* e, più sotto, *«ripeti la query fuori da PIE, con
  il ponte acceso»*.

---

## 4. Cosa il kit chiedeva e non è stato fatto

| Chiesto | Fatto? | Perché |
|---|---|---|
| `ED0` — freeze delle fonti, owner per 15/15 `S` | ❌ | L'oggetto non è nel repository (`R3`). Gli owner sono stati **misurati** (`R7`), che è la parte eseguibile della richiesta |
| `ED1` — harness Editor comune, dry-run MCP e umano | ❌ | `NOT RUN` — §7 |
| `ED2…ED8` — esecuzione `S01…S15` | ❌ | Bloccati su `ED0`/`ED1` |
| Creare la roadmap in `docs/` | ❌ | **Deliberato**: `R2`, la roadmap esiste |
| Commit `docs(schematics): add editor execution roadmap for S01-S15` | ❌ | Conseguenza della riga sopra |

---

## 5. I tre contributi che si salvano

Nessuno dei tre esiste oggi, e nessuno richiede una roadmap nuova per essere adottato.

### C1 — `Oracle: CANON | OBSERVED` 🔑

È il contributo migliore del kit, e nasce dal suo difetto peggiore. Il caso `Action.Sprint` (`R4`) mostra
che *«la schematica corrisponde al gioco»* e *«la schematica corrisponde al canone»* sono **due domande
diverse**, e che il repository ha almeno un punto in cui divergono per un debito dichiarato.

```text
Oracle: CANON | OBSERVED
KnownDivergence:
  Action.Sprint — pre-Blast per arretrato #641 / D-116 (E38, v0.2)
```

Con `Oracle: CANON` e la divergenza dichiarata, una verifica passa **e** registra il debito. Senza, il
primo operatore che incontra la divergenza la risolve da solo, in un verso o nell'altro.

⚠️ **Owner naturale**: [`roadmap-esecuzione-pie-2026-09-03.md`](roadmap-esecuzione-pie-2026-09-03.md),
che possiede già *«in che ordine e cosa manca»*. Non è stato scritto qui: aggiungerlo è una modifica al
suo contratto, e appartiene a chi lo possiede.

### C2 — Il falsificatore per voce

Il registro PIE ha *«esito atteso»*; non ha un campo *«cosa lo falsifica»*. È la differenza fra una
verifica che si supera guardando la cosa giusta e una che si supera guardando **una barra già accesa**.
Il kit lo chiede per tutte e quindici le `S`, e la richiesta è sana anche fuori dal suo contesto.

### C3 — La sequenza «la fase prima di tutto»

`S05 → S01 → S04` — *l'ordine di Resolution si verifica per primo perché condiziona quasi tutte le altre
letture* — è una tesi difendibile, e nessuna sequenza esistente la dichiara. ⚠️ Vale **dopo** `C1`: oggi
quel primo passo produrrebbe il verdetto sbagliato.

---

## 6. Due difetti trovati per strada, e corretti qui

Nessuno dei due viene dal kit: sono emersi misurando i suoi owner.

### D1 — Otto rinvii vivi a un owner rimosso, in quattro file

```bash
grep -rn -i "editormap" docs --include=*.md --include=*.yaml | grep -v archive | grep -v research
```

Le occorrenze sono molte; quasi tutte sono **sane** — referti datati che citano la EditorMap come
provenienza, o documenti che ne dichiarano la rimozione (`CONTEXT_INDEX.md`, `roadmap-checkpoint.md`,
`roadmap-v0.1.md`, `spec-tactical-designer.md`, e `roadmap-editor.md`, che porta la correzione dodici
righe sotto la propria tabella). ⛔ **I rinvii ancora operativi, che presentano la EditorMap come owner
corrente, erano cinque in tre file:**

| File | Dove | Cosa affermava |
|---|---|---|
| `test-manuali-pie.md` | intestazione (×2) | *«quale voce affrontare e quando lo dice la EditorMap»* |
| `test-manuali-pie.md` | §*La sequenza* | *«quella e' la EditorMap, generata da editor-sessions.yaml»* |
| `test-manuali-pie.md` | §subset `RELEASE-V01` | *«le 55 voci orfane … sono contate in editormap.shortlist.md»* |
| `test-manuali-pie.md` | voce `PIE-V01-ARENA` | *«Seduta U1 di editormap.shortlist.md»* |
| `editor-sessions.yaml` | **riga 1**, il titolo | *«Sedute in editor — la sorgente della EditorMap»* |
| `asset-map.md` | §*chi è l'owner* | *«editor-sessions.yaml, reso in editormap.shortlist.md»* |
| `scenario-map.md` | §orfane | *«il conteggio … vive in editormap.shortlist.md»* |

🔴 **Questa sezione ha sbagliato il proprio conteggio due volte, e le due correzioni sono il reperto.** La
prima stesura ne dichiarava **tre**, tutti in un file — cioè ripeteva un piano più su il difetto che
contesta: la correzione del 2026-08-28 aveva guardato **una voce** invece del file, questa guardava **un
file** invece del repository. La seconda ne dichiarava **cinque**, e ne mancavano due nello stesso
`test-manuali-pie.md`, trovati solo perché una review ha chiesto di rifare la misura. Sono **otto**, in
quattro file. ⚠️ Il caso peggiore resta `editor-sessions.yaml`: **il titolo, tre righe sopra la nota che
questo pass ha aggiunto** — corretto un numero stantio nel file, lasciando in testa il rinvio morto.

⛔ **Due occorrenze NON sono state toccate, ed è deliberato**: `roadmap-editor.md:13`, una tabella dentro
un blocco che il file stesso dichiara *«resta come provenienza»* e che porta la rimozione dodici righe
sotto; e `editor-sessions.yaml:1795`, dentro le `notes:` di una seduta, che raccontano come la vista
classificava `U24` quando esisteva. Riscriverle falsificherebbe la storia —
[`docs/README.md`](../../README.md) §*Tag*: *«uno storico che descrive un mondo scomparso non è un difetto
da correggere»*. Il comando che le isola, e che chiunque può rieseguire:

```bash
grep -rn -i "editormap" docs --include=*.md --include=*.yaml \
  | grep -vE "docs/(archive|research|generated)/|docs/roadmap/plans/" \
  | grep -viE "rimoss|uscit|non esiste|D-181|corretto il|diceva|viveva|aveva gia"   # -> 2
```

La **EditorMap** (`editormap.shortlist.md`) e il generatore che la produceva sono usciti dal repository il
**2026-08-21** con `D-181`, e [`editor-sessions.yaml`](../editor-sessions.yaml) lo dichiara di sé:
*«Questo file non ha più una vista, e non ha più un consumatore. Nessuno script legge questo file.»*

🔴 **È il difetto che questo stesso file documenta di aver già corretto una volta.** La voce
`PIE-SCEN-COMPOSER` porta la nota: *«Questa frase rimandava alla EditorMap, che D-181 ha rimosso il
2026-08-21: rinviare a un owner che non esiste più significa che la voce non viene schedulata da nessuno
— corretto il 2026-08-28.»* La correzione fu fatta **su una voce**, e i rinvii dell'intestazione e della
sezione *«La sequenza»* — che è quella che possiede l'affermazione — sono rimasti.

✅ **Corretti in questo pass tutti e otto**: puntano ora a [`editor-sessions.yaml`](../editor-sessions.yaml)
per i **dati**, a [`test-manuali-pie.md`](../../technical/test-manuali-pie.md) per l'**esito** e a
[`roadmap-esecuzione-pie`](roadmap-esecuzione-pie-2026-09-03.md) per l'**ordine**. ⛔ **Non toccati, ed è
deliberato**: la nota dentro la voce `PIE-SCEN-COMPOSER` e i documenti che dichiarano la rimozione — sono
provenienza, e riscriverli cancellerebbe la traccia di come il difetto è stato scoperto.

⚠️ **Cosa nessun gate vede.** `doc-links.ts` dichiara di non coprire *«i percorsi citati in prosa o dentro
inline code»*, ed è la forma della maggior parte degli otto: un nome fra backtick in un commento `.yaml` o in
una riga di prosa non è un link, quindi il gate esce `0` mentre l'affermazione è falsa. Il prossimo rinvio
morto si troverà allo stesso modo — a mano, misurando un owner per un'altra ragione.

⚠️ **Nota di metodo, misurata su sé stessa**: la prima stesura di questo paragrafo citava quella nota per
**numero di riga**. La correzione qui sopra l'ha spostata da `1393` a `1396` — un riferimento per riga
invecchia nel commit che lo scrive, ed è la stessa famiglia del difetto che questa sezione documenta.

### D2 — `editor-sessions.yaml` dichiara 27 sedute, e sono 48

```bash
grep -oE "^  - id: U[0-9]+" docs/roadmap/editor-sessions.yaml | sort -u | wc -l   # -> 48, zero duplicati
git log -1 -S "27 sedute con passi" -- docs/roadmap/editor-sessions.yaml          # -> d671df47, 2026-08-21
git show d671df47:docs/roadmap/editor-sessions.yaml | grep -cE "^  - id: U"       # -> 27
```

⚠️ **Il numero era esatto il giorno in cui è stato scritto** ed è invecchiato: 21 sedute in 16 giorni. La
somma per `block` concorda (`29+7+4+3+3+2 = 48`), e il parser YAML — `yaml.safe_load(...)['sessions']` —
ne conta **48**.

🔴 **Ma non è una scoperta di questo pass, e la prima stesura lo presentava come tale.**
[`primary-fix-…-2026-09-05.md`](primary-fix-dual-roadmap-spec-panel-2026-09-05.md) pubblica **48 sedute**
già il 2026-09-05, in due punti — ed è un referto che questo documento cita **quattro volte**, `R1`
compreso. ⚠️ Il valore corretto era quindi in casa da un giorno, e il file di dati continuava a dire `27`:
il difetto non era che nessuno avesse misurato, ma che **la misura non era tornata all'owner**. È
esattamente l'anti-duplicazione su cui `R1` è costruito, mancata da chi la contestava.

🔑 **E rafforza `R2`**: il file cresce di una seduta ogni 18 ore e non ha né vista né consumatore. Il
problema dell'esecuzione Editor non è che manchi un piano — è che ne esistono tre e nessuno viene reso.

✅ **Corretto in questo pass**, con i tre comandi accanto al numero invece del solo numero. ⚠️ **La
correzione ri-arma la trappola che disinnesca**, e va detto: `48` è un derivato scritto a mano in un file
senza gate, quindi sarà stantio entro un giorno. La forma che non invecchia è il **comando**, ed è per
questo che è stato scritto nel commento accanto al valore — chi lo rilegge può rifare la misura invece di
crederle.

---

## 7. NOT RUN

- **`ED1` e ogni `Sxx`**: non eseguiti. L'oggetto non esiste (`R3`) e la roadmap non è stata creata (`R2`).
- **Unreal Editor / PIE**: nessuna apertura da questo workflow. `UnrealEditor-Cmd.exe` era **vivo e
  posseduto da un'altra sessione** all'inizio della run — ragione per cui questo pass è stato eseguito in
  un **worktree separato**: scrivere in `docs/` nella working directory condivisa avrebbe reso
  `NON REGISTRABILE` la misura altrui.
- **Capability MCP (kit §3)**: `NOT RUN`. Due ricerche sugli strumenti esposti non hanno restituito alcun
  tool Unreal in questa sessione. ⚠️ **Non è una prova di assenza** — `CLAUDE.md` §5: *«una risposta vuota
  non è un no»* — e converge col rilievo `R5` di
  [`primary-fix-dual-roadmap-spec-panel-2026-09-05.md`](primary-fix-dual-roadmap-spec-panel-2026-09-05.md),
  che ieri misurò *«tre porte a HTTP 000, nessun tool `mcp__unreal*` esposto»*. La fattibilità di
  `MCP-EDITOR` resta **non misurata**, non smentita.
- **`rt-suite.ps1`**: non eseguita, e non serviva: write-set di soli documenti, **nessuna riga di
  `Source/` toccata**.

---

## 8. Prossimo passo

**Uno.** Portare `C1` — `Oracle: CANON | OBSERVED` con la divergenza `Action.Sprint` — a
[`roadmap-esecuzione-pie-2026-09-03.md`](roadmap-esecuzione-pie-2026-09-03.md), che è l'owner della
sequenza di esecuzione. È l'unico dei tre contributi che protegge da un verdetto sbagliato invece di
aggiungerne uno nuovo.
