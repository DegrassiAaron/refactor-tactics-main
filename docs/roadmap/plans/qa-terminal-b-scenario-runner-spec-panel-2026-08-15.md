# QA Terminal B — spec panel sul prompt «Scenario Runner e automazione»

> `CURRENT` · **Stato**: revisione chiusa, **applicata** — il documento recensito è versionato, e le
> correzioni sono nel commit che accompagna questo referto ·
> **Data**: 2026-08-15
> **HEAD della revisione**: `964e1b57` (`origin/main`), worktree `D:/rt-spatial`, working tree pulito
> **Oggetto**: [`../../technical/qa-prompt-terminal-b-scenario-runner.md`](../../technical/qa-prompt-terminal-b-scenario-runner.md)
> — **v2**, 199 righe, versionato con PR #900 (`79f61f92`, antenato di `main`).
> **Panel**: Wiegers (lead) · Crispin · Adzic · Cockburn · Nygard · Fowler
> **Modo**: critique · **Focus**: requirements + testing
> Gemello del referto di Terminal A
> ([`qa-terminal-a-determinism-spec-panel-2026-08-15.md`](qa-terminal-a-determinism-spec-panel-2026-08-15.md)),
> che il documento recensito cita come autorità per tre correzioni condivise.

## 0. Cosa è questo documento

Il file recensito è il **prompt operativo** di una sessione Claude — uno di tre, pensati per girare in
parallelo su Test/QA. La v1 era untracked ed è andata persa; la v2 è stata scritta ex novo e versionata.

Il panel ha applicato una regola sola prima di parlare: **ogni affermazione verificabile del documento è
stata misurata**, e nessun giudizio poggia su ciò che il prompt afferma di sé.

⚠️ **Il panel ha sbagliato una misura e la registra qui, perché è la stessa classe di errore che ha
trovato nel documento.** Il primo passaggio concluse «solo due track hanno un blocco `writable`, nessuna
copre `ScenarioHarness/`», e ne ricavò il finding più grave del referto: *il mandato non è eseguibile*.
Era falso. Il comando era `grep -A 12 "^    writable:" | head -80`, e `head -80` tagliava l'output prima
della terza track. Rimisurato con un parser che percorre tutte le righe: **cinque track, tre con
`writable`**, e la prima è `spatial`, che porta il `mandate` di questo stesso documento e ha
`Source/RefactorTactics/ScenarioHarness/` nel proprio write-set. Il finding non è caduto — si è
**capovolto**, e nella forma corretta è più utile. La lezione vale per chiunque legga: *un `head` su un
output di misura non tronca la stampa, tronca la conclusione.*

## 1. Le affermazioni, misurate

Tutte le misure su `origin/main`. Il documento si ancora a `79f61f92`, verificato antenato di `964e1b57`;
nessuna delle righe qui sotto cambia fra i due commit.

| Affermazione della v2 | Misura | Esito |
|---|---|---|
| 13 file in `ScenarioHarness/` | 13, nomi identici | ✅ |
| `.github/workflows/` assente | assente | ✅ |
| `scripts/` = 10 script Python, nessun runner C++ | 10 `.py` | ✅ |
| `feature_registry.py` e i suoi **tre** test | 3 file `test_feature_registry_*.py` | ✅ |
| Zero `TestAgent` / `AIAgent` / `AiAgent` in `Source/` e `docs/` | zero | ✅ |
| `Scenarios/`, `scenario-map.md`, `feature_registry.py` sono `integration_only` | tutti e tre presenti fra le 27 voci | ✅ |
| `Scenarios/` = «76 file JSON in `Combat/`, `Movement/`, `Spec/`, `Visual/`» | quelle quattro ne contengono **74** | ❌ |
| «**76 scenari** versionati» | uno dei 76 è `_redirects.json`, che scenario non è | ❌ |
| Comandi console «**reali**», tre | `RTTestConsole.cpp` dichiara **cinque** nomi | ❌ |
| `RTTestResult.h` contiene `ERTTestOutcome` | è dichiarato in `RTTestScenario.h:22` | ❌ |
| «Esiti: `PASS` · `FAIL` · `ERROR` sono **tre**» | l'enum ha **quattro** valori | ❌ |
| `ScenarioHarness/` «non assegnato a nessuna track» | è nel `writable` di `spatial`, che porta il `mandate` di questo documento | ❌ |

Sei affermazioni su dodici non reggevano. **Nessuna è un refuso**: tutte e sei nascono dallo stesso
metodo, e il §4 lo isola.

### 1.1 Le misure, per esteso

```sh
# ripartizione dei JSON — il totale nudo dà 76 e nasconde entrambi gli scarti
git ls-tree -r --name-only origin/main -- Scenarios/ | grep '\.json$' \
  | sed 's|Scenarios/||; s|/.*||' | sort | uniq -c
#   1 _redirects.json          ← non è uno scenario
#   9 Combat                   ┐
#   6 Movement                 │ 74 nelle quattro categorie
#   1 RT_Showcase_Relay_v01.json  ← scenario, ma alla radice
#  38 Spec                     │
#  21 Visual                   ┘

grep -n 'TEXT("rt\.' Source/RefactorTactics/ScenarioHarness/RTTestConsole.cpp
#  25: rt.Test.Scenario     TAutoConsoleVariable — auto-run all'avvio partita
#  51: rt.Map.Source        TAutoConsoleVariable — scavalca MapSource del GameMode
# 146: rt.Test.List         ┐
# 151: rt.Test.Run          │ i tre che la v2 elencava
# 156: rt.Test.DumpResult   ┘
```

```cpp
// Source/RefactorTactics/ScenarioHarness/RTTestScenario.h:22
enum class ERTTestOutcome : uint8 { Pass, Fail, Error, Blocked };
// RTTestResult.h:128 → case ERTTestOutcome::Blocked: return TEXT("BLOCKED");
```

```yaml
# docs/roadmap/parallel-batch.yaml — track `spatial`
spatial:
  status: IDLE
  mandate: docs/technical/qa-prompt-terminal-b-scenario-runner.md
  writable:
    - Source/RefactorTactics/ScenarioHarness/
  derives_from:
    - source: Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp
      regenerates: [docs/roadmap/project-graph.json, docs/roadmap/scenariomap.shortlist.md]
```

## 2. Findings

### 🔴 C1 — L'enum degli esiti ne ha quattro, e il vincolo §6 legittimava di ignorare il quarto

**CRISPIN**: la v2 scriveva al §2 «`PASS` · `FAIL` · `ERROR` sono **tre** […] Non collassarli in due», e
lo promuoveva a vincolo al §6. `Blocked` esiste, è commentato con la propria ragione d'essere — versionare
uno showcase *prima* che tutti i suoi sistemi esistano — ed è stato aggiunto in coda perché i valori
precedenti non cambiassero numero.

Un vincolo che enumera tre stati e ne vieta il collasso *a due* dichiara implicitamente che l'insieme è
chiuso a tre. Chi scrive un report, un parser di `result.json` o un aggregatore conforme a quel mandato
tratta `Blocked` come fuori contratto — e `Blocked` è **l'esito che tace**: uno scenario che non ha mai
eseguito la parte interessante non è rosso, e nessuno se ne accorge.

**NYGARD**: `FRTTestResult::Outcome` è inizializzato a `Error`, il che è corretto. Ma un contatore
`Passed/Failed` derivato da un mandato a tre stati assorbe i `Blocked` da una parte o dall'altra, e
nessuna delle due è giusta.

✅ **Applicato**: §2 riscritto con la tabella a quattro valori, la ragione d'essere di `Blocked` e
l'avvertenza che è l'esito da trattare con più sospetto; §6 riformulato su quattro; §4 esteso — decidere
che uno scenario è bloccato è una lettura di capability, non un giudizio dell'agent.

### 🔴 C2 — Il §1 dichiarava non assegnato il proprio write-set

**COCKBURN**: la v2 apriva la tabella di ownership con «`ScenarioHarness/` — non assegnato a nessuna
track — va **richiesto** in `writable`», e chiudeva con «trova la tua track […] se non esiste, fermati e
chiedi la riallocazione».

Misurato: `parallel-batch.yaml` assegna a `spatial` **sia** il `mandate` di questo documento **sia**
`Source/RefactorTactics/ScenarioHarness/` nel `writable`, e la nota della track argomenta la scelta
citando proprio il fatto che il path «non era nel `writable` di nessuna track». Le due cose sono state
decise lo stesso giorno; il prompt non è stato riconciliato con l'assegnazione che lo riguarda.

**WIEGERS**: e resta un residuo vero, che la formulazione sbagliata copriva. `spatial` è `IDLE`, e il
batch è esplicito: *un `mandate` non rende `ACTIVE` la track*; `status` passa ad `ACTIVE` quando una
sessione parte **con una issue**, e `ACTIVE` con `issue: null` è una contraddizione decidibile. ∴ il primo
passo del mandato non è scrivere codice — è **avere una issue**. La v2 non lo diceva in nessun punto.

✅ **Applicato**: riga di tabella corretta con lo stato reale; §1 riscritto con il blocco YAML misurato,
la regola `IDLE`-con-mandato e il passo 0 «abbi una issue» in testa al §5; `Start` §2 riscritto.

🔁 **Rimisurato a chiusura del referto**, perché `parallel-batch.yaml` è cambiato durante la revisione
(`964e1b57` → `a7c4a799`): la track `spatial` è **invariata** — `IDLE`, stesso `mandate`, stesso
`writable`. È cambiato il contorno: `simulation` è passata ad `ACTIVE` con il mandato A e cinque path, e
le track con `writable` sono quattro invece di tre. Nessuna delle due cose tocca ciò che il §1 corretto
afferma.

### 🔴 C3 — La lista di lavoro indicata non conteneva un item iniziabile

**ADZIC**: il §5.2 diceva «Leggi §9 e §10 […] Quella è la lista», e `Start` §3 restringeva al solo §9. Ho
aperto il §9 della spec owner. Sono quattro voci:

| Voce §9 | Perché non parte |
|---|---|
| Assertion su HP, scudo, stati, TurnLog | *«da aggiungere quando uno scenario le richiede»* — ma `Scenarios/` è `integration_only`. **Stallo** |
| Intent diversi dal movimento | precondizione dichiarata, non un task |
| Politica per le Fast Reaction | dipende da **E14**, non atterrato |
| Nessun bypass | invariante permanente — si rispetta, non si chiude |

Zero su quattro. Il lavoro eseguibile sta al **§10** — `mapId`, `facing`, `intents[].ability`,
`surfaces[]`/`structures[]`/`objective`, `reactionPolicy[]`, `ruleset`, i tre modi — ed è tutto dentro il
`writable` della track. `Start` §3 mandava il lettore esattamente dove non si parte.

**FOWLER**: c'è un secondo anello, più sottile. Il §10.5 chiede che `HEADLESS`/`FAST`/`VISUAL` diano lo
stesso esito logico, e che l'equivalenza *sia essa stessa un test*. Quel test ha bisogno di scenari, che
questo terminale non può scrivere. La forma del vincolo di ownership rende non verificabile l'invariante
più importante dello schema target, a meno di proporre i due scenari in handoff insieme al codice.

✅ **Applicato**: §5 riscritto con la tabella del deadlock e il puntamento al §10; `Start` §3 corretto con
il divieto esplicito sul §9; aggiunta la nota sui due scenari `Visual vs Fast` e `Fast vs Headless` da
proporre insieme ai modi; aggiunto un criterio di chiusura del mandato, che la v2 non aveva.

### 🟡 M1 — «76 scenari in quattro cartelle»: il numero regge, la frase no

9 + 6 + 38 + 21 = **74**. Gli altri due sono `RT_Showcase_Relay_v01.json`, che è uno scenario ma sta alla
radice, e `_redirects.json`, che scenario non è. Due difetti in una riga: la ripartizione non somma, e il
totale conta un non-scenario.

**ADZIC**: il punto che conta è il comando offerto al §7 — `find Scenarios -type f -name "*.json" | wc -l`
— che restituisce `76` e quindi **conferma il numero sbagliato**. Un esempio che non può falsificare
l'affermazione che accompagna non è una verifica, è una rassicurazione.

✅ **Applicato**: riga di tabella con la ripartizione esplicita; §7 sostituito con la forma per cartella.

### 🟡 M2 — La regola di precedenza faceva vincere un owner obsoleto

**FOWLER**: §0.4 stabiliva «se questo prompt e quella spec divergono, vince la spec». Applicata alle due
righe misurate:

| | Prompt v2 | Owner `test-automatico-unreal.md` | Codice |
|---|---|---|---|
| Scenari | 76 | **cinque** (§3, «Al 2026-08-08») | 76 file |
| Esiti | tre | **tre** (§6) | **quattro** |

Su entrambe la precedenza dichiarata mandava il lettore sul valore sbagliato. Mancava la clausola di
freschezza: l'owner vince sulle **decisioni** — perché gli esiti sono distinti, perché il `seed` non fa
niente, perché il JSON non è `.uasset` — non sui **conteggi**, che invecchiano da soli.

**WIEGERS**: e la regola giusta il prompt ce l'aveva già scritta due sezioni dopo, al §7: «Non copiare
conteggi da questo documento: rimisurali». La applicava a sé stesso e non all'owner.

✅ **Applicato**: §0.4 riscritto con la distinzione decisioni/conteggi e le due divergenze misurate; §8
esteso con una voce «Divergenze dall'owner».

### 🟡 M3 — «Comandi reali, non inventarne di nuovi»: ne elencava tre su cinque

**NYGARD**: i due omessi sono `TAutoConsoleVariable`, non `FAutoConsoleCommand` — ed è per questo che il
grep offerto al §7 non li vedeva. Ma `rt.Test.Scenario` **è** il meccanismo di auto-run, cioè metà del
titolo del documento («Scenario Runner **e automazione**»), e `rt.Map.Source` porta nel proprio commento
un dato operativo che una sessione pagherebbe caro per riscoprire:

> ⚠️ *Da riga di comando serve `-dpcvars=`, non `-ExecCmds=`* — misurato sul pacchettizzato il
> 2026-08-10. `-ExecCmds` gira DOPO l'inizializzazione, quando il GameMode ha già allestito la partita:
> la variabile viene impostata e non serve a niente, senza un errore che lo dica.

Un mandato che vieta di inventare comandi e omette quelli esistenti spinge a inventarne.

✅ **Applicato**: §2 esteso con le due console variable e l'avvertenza `-dpcvars`; §7 con
`grep 'TEXT("rt\.'` al posto di `grep FAutoConsoleCommand`.

### 🟡 M4 — I comandi del §7 non giravano nell'ambiente del lettore

**CRISPIN**: `find | wc -l`, `ls`, `grep -rln` sono POSIX; il blocco era etichettato ```` sh ```` senza
dire quale shell, e l'ambiente di lavoro è Windows con PowerShell come shell primaria. In PowerShell `wc`
e `find` non esistono, e `Measure-Object -Line` **scarta le righe vuote**: un conteggio fatto lì è più
basso del vero senza dirlo. Ed è il **primo** passo del §5.

✅ **Applicato**: blocco §7 riscritto in forme `git ls-tree`, che girano in entrambe le shell e misurano
una revisione dichiarata invece del working tree — che su un worktree di feature non è `main`. Aggiunta
l'avvertenza sulla shell.

### 🟡 M5 — La mappa del §2 metteva `ERTTestOutcome` nel file sbagliato

`RTTestResult.h` lo usa; è dichiarato in `RTTestScenario.h:22`. Minore in sé, ma è la mappa con cui il
lettore decide quale file aprire — ed è lo stesso file in cui vive C1.

✅ **Applicato**: mappa dei file corretta, con `(4 valori)` accanto all'enum.

### 🟢 Minori

| # | Rilievo | Stato |
|---|---|---|
| m1 | Il §4 chiedeva quattro risposte scritte senza dire **dove atterrano**, mentre il §8 impone il path di destinazione agli handoff | ✅ atterrano nell'handoff, sotto un titolo dichiarato |
| m2 | §3.3 diceva «portala al Decision Log» senza path — e senza dire che è `integration_only` | ✅ path aggiunto, con la conseguenza |
| m3 | §8 elencava nove voci allo stesso livello, tre delle quali condizionali | ✅ marcate *(se applicabile)* |
| m4 | Nessun criterio di chiusura del mandato | ✅ aggiunto in coda al §5 |
| m5 | **Nuovo**: i tre mandati QA non sono in un `writable` né in `integration_only` — sono file **non assegnati**, e il §1 di questo stesso documento dice *file non assegnato = STOP* | ✅ registrato in tabella §1 e in `Start` §5, poi **risolto nel batch** lo stesso giorno: vedi §5 |

## 3. Cosa la v2 faceva bene, e va conservato

**WIEGERS**: il documento è **strutturalmente forte** dove la maggior parte dei mandati è debole, e questo
è il motivo per cui il panel lo ha corretto invece di riscriverlo.

- Distingue **permesso** da **conseguenza**: il blockquote sulle viste generate non chiede un permesso, dice
  cosa si rompe a valle.
- Vieta di implementare in attesa di una decisione (§3), invece di limitarsi a scoraggiarlo.
- Chiede un **soggetto** prima di costruirlo (§4), con una domanda-gate che funziona: *«come si falsifica
  il suo output? (se non si falsifica, non è un test)»*.
- Impone il path di destinazione a un handoff — *un handoff senza un file dove atterrare non ha un lettore*.
- Dichiara che l'AI non decide l'esito. Resta vero, e la v2.1 lo estende a `Blocked`.

## 4. La firma comune dei sei difetti

**CRISPIN**: tutte e sei le affermazioni cadute hanno la stessa origine. Il documento predica «misura, non
assumere» ed è stato **scritto per trascrizione**: `76` dall'output di `wc -l`, «tre esiti» dalla spec
owner, «tre comandi» dalla porzione di file che `grep FAutoConsoleCommand` raggiungeva, «non assegnato»
dallo stato del batch *prima* dell'assegnazione decisa lo stesso giorno.

⚠️ **E il difetto non è di questo mandato: è dei tre.** Mentre il panel lavorava, `origin/main` è avanzato
da `964e1b57` ad `a7c4a799` con una correzione al mandato A, arrivata per un'altra strada e con lo stesso
titolo che avrebbe potuto avere qualunque riga di questo referto — *«il Repeat x100 esisteva, e il mandato
diceva che non serviva»* (`7f8b5703`). Due sessioni indipendenti, due mandati diversi, un solo errore:
**il documento afferma l'assenza di una cosa che c'è.** Il che rende la regola qui sotto una proprietà del
formato «prompt operativo», non un incidente di scrittura.

**ADZIC**: ∴ la lezione riutilizzabile, e vale oltre questo documento —

> **Un comando di verifica scritto dopo l'affermazione non la verifica: la ripete.**
> Misura a una granularità **più fine** di quella in cui l'affermazione è scritta. Un totale non falsifica
> una ripartizione; `FAutoConsoleCommand` non falsifica «tre comandi»; la spec non falsifica un enum; e un
> `head` non falsifica «nessuna track».

L'ultima clausola è il panel che parla di sé: il finding C2 è nato da un output troncato, ed è stato
capovolto rimisurando. La regola non ha eccezioni per chi la scrive.

## 5. Ciò che questo referto **non** ha applicato

✅ **Le due correzioni all'owner sono state applicate**, su decisione esplicita dell'autore del repository
dopo la chiusura del referto. [`../../technical/test-automatico-unreal.md`](../../technical/test-automatico-unreal.md)
era indietro su due righe misurate:

| Sezione | Diceva | Misura | Esito |
|---|---|---|---|
| §3 «Dove vivono gli scenari» | «Al 2026-08-08 ne esistono **cinque**, tutti `Movement.*`» | 76 file JSON, 74 in quattro categorie | ✅ ripartizione per cartella, con lo scarto dichiarato |
| §6 «`PASS` · `FAIL` · `ERROR` — e perché sono tre» | tre esiti, e ne **argomenta** tre | `ERTTestOutcome` ne ha quattro | ✅ riscritto su quattro, con la ragione d'essere di `Blocked` |
| §8 «Console e auto-run» | quattro nomi | cinque: mancava `rt.Map.Source` | ✅ aggiunto, con l'avvertenza `-dpcvars=` |
| §9 «Requisiti aperti» | quattro voci, senza dire che nessuna è iniziabile | stallo misurato (vedi C3) | ✅ annotato, con il rimando al §10 |

Il §6 era il caso serio: non un conteggio scaduto, ma una sezione che *spiegava perché sono tre* mentre il
codice ne ha quattro — e il quarto porta nel proprio commento la motivazione che la spec non riportava.
⚠️ **E la contraddizione era interna al file**: il §4.2 usava già `BLOCKED` come esito, alla riga 105.
Nessun gate rilegge una spec `as-built`, e questa è invecchiata addosso a sé stessa per una settimana.

∴ l'intestazione del file ora **data le sezioni separatamente** invece di dichiarare un unico
«allineata al codice il …», che è la forma in cui l'invecchiamento diventa invisibile.

🔴 **E una nota sull'assegnazione di questo referto stesso.** Al momento della revisione i tre mandati QA
— `qa-prompt-terminal-{a,b,c}` — erano nominati in `parallel-batch.yaml` dal campo `mandate:`, ma i loro
**file** non comparivano in nessun `writable` né fra le allora 27 voci di `integration_only`. Erano file
non assegnati, e il §1 del documento appena corretto dice *file non assegnato = STOP*. Le correzioni di
quel giorno sono avvenute su decisione esplicita dell'autore del repository, registrata qui perché sia
visibile e non dedotta.

✅ **Chiusa lo stesso giorno, e non come l'avevo proposta.** Ogni mandato è entrato nel `writable` della
track che lo esegue — B in `spatial`, A in `simulation`, C in `content_editor` — con i worktree
dichiarati: `D:/rt-spatial`, `D:/rt-simulation`, e per C il campo nuovo `mandate_worktree: D:/rt-client`,
perché il worktree della seduta U1 è occupato da un write-set che tocca `parallel-batch.yaml` stesso.

⏱️ **E A è già uscito, poche ore dopo.** `#915`/`#578` hanno chiuso, `simulation` è tornata `IDLE` e ha
perso il `mandate`: `qa-prompt-terminal-a-determinismo.md` è di nuovo **non assegnato**. Non è un errore
della decisione — è il suo limite, scoperto al primo caso: *legare un mandato al `writable` di una track
lo lascia scoperto quando la track chiude*. Il batch adesso registra la domanda invece di lasciarla
implicita — se il mandato riapre torna alla track che riparte, se resta chiuso va dove sta la spec che
governa, cioè `integration_only` — e chiede che si decida **alla riattivazione, prima di scrivere**.
Il criterio è *chi esegue un mandato lo corregge quando lo misura sbagliato*, ed era già successo due
volte in un giorno: `7f8b5703` sul mandato A, `077d0b47` su questo.

⚠️ **La spec del harness invece no, ed è la parte che avevo sbagliato a proporre.** Avevo suggerito di
darla a `spatial`, che possiede `ScenarioHarness/` e l'aveva appena corretta. Risposta misurata: la
usano **più processi in parallelo** — la citano per nome il mandato A (4 volte) e il mandato B (5), due
track distinte, più `piano-canonico-mvp.md`, che è nel `writable` di una terza. Assegnarla a una sola
track avrebbe tolto alle altre il diritto di correggere un documento che leggono come owner. È entrata in
`integration_only`, che ora ha 28 voci. ∴ **la distinzione non è fra assegnato e non assegnato, ma fra
posseduto da uno e riconciliato da tutti** — e un documento che invecchia mentre più sessioni lo leggono
come autorità appartiene alla seconda categoria.

## 6. Giudizio

*Valutazione del panel, non una misura.*

| Dimensione | v2 | v2.1 | Nota |
|---|---|---|---|
| Chiarezza di scrittura | 8.5 | 8.5 | precisa, gerarchizzata, senza prosa di riempimento |
| Disciplina di ownership | 8 | 8.5 | il pezzo migliore; ora include sé stesso fra i file non assegnati |
| **Accuratezza fattuale** | **4** | 8 | sei affermazioni su dodici erano false; le sei correzioni portano la misura |
| **Eseguibilità** | **3** | 8 | c'è un passo 0 (la issue), e la lista di lavoro indicata contiene lavoro |
| Verificabilità dei comandi | 4 | 8 | i comandi ora falsificano invece di confermare, e girano in entrambe le shell |

**Ordine in cui le correzioni sono state applicate**: C1 (vincolo attivo e sbagliato) → C2 + C3 insieme
(senza issue non si scrive, senza il §10 non c'è cosa scrivere) → M1/M3/M5 con la sostituzione in blocco
del §7, che chiude anche M4 → M2 → minori.
