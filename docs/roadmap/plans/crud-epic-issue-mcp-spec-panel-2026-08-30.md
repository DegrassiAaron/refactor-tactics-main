# CRUD Epic / Issue + GitHub MCP + Unreal MCP — spec panel

> `CURRENT` · **Stato**: revisione chiusa · **`R0` eseguito per intero** su conferma d'autore — epic
> riconciliate (§ [*cosa è stato riconciliato*](#r0--cosa-è-stato-riconciliato)) e le due sweep
> (§ [*orfane e duplicati*](#r0--le-due-sweep-orfane-e-duplicati)) · **Data**: 2026-08-30
> **HEAD**: `20d59973` · branch `diag/1665-istanze-board` · `origin/main` `417ecfb5` dopo `git fetch --prune`
> **Sorgente revisionata**: `Claude_RefactorTactics_v0.1_CRUD_Epic_Issue_MCP.md` (**1463** righe), arrivata
> `untracked` a radice, archiviata a fine sessione in
> [`../../archive/src/handoff/2026-08-30-crud-epic-issue-mcp.md`](../../archive/src/handoff/2026-08-30-crud-epic-issue-mcp.md).
>
> **Cosa possiede**: il verdetto misurato sul kit, difetto per difetto, con il comando che l'ha trovato.
> **Cosa non possiede**: autorità. Il kit non autorizza sé stesso — il guardrail di
> [`CLAUDE.md`](../../../CLAUDE.md) §7 dice che *un handoff/audit non è autorità e non autorizza da solo a
> implementare tutto ciò che contiene*. Di tutto ciò che chiede è stato eseguito **solo `R0`**, dopo conferma
> esplicita: **una** mutazione GitHub, il corpo di `#14`. Zero issue create, chiuse o riaperte.

## Il verdetto, in breve

✅ **È il kit più accurato arrivato finora su questo terreno.** Tutte e **29** le ancore GitHub del §5
esistono, con i titoli che il kit attribuisce loro: zero numeri inventati. I **10** percorsi del preflight
§4 esistono tutti. La rimozione del Feature Registry (§3) è reale. Il divieto «*considerare replay =
resimulation*» (§27) è letteralmente la tesi di
[ADR-0009](../../decisions/adr-0009-replay-logico-canonico.md). La baseline «`G1–G14`, ma riverifica»
(§10/R7) è **giusta**: il DoD arriva a `G15`, ma `G15` è ritirato da `D-181` e il suo numero non si riusa —
i gate vivi sono quattordici.

✅ **Le sue tre premesse epistemiche reggono e sono misurabili.** «*Search before create*» (§2), «*il parent
`#14` può essere indietro*» (§5), «*se la label non esiste non inventarla*» (§10/R4): tutte e tre vere.
`#14` è aggiornata al **2026-08-25** e non cita **quattro** delle quattordici epic `[EPIC v0.1]` aperte.
Nessuna label `review`/`human`/`ready`/`blocked` esiste nel repository.

🔴 **E c'è un difetto che invalida un capitolo intero: il §13 prescrive scritture che il ponte MCP non ha.**
`Plugins/RTDeveloperTools` espone **5** `UFUNCTION(meta = (AICallable))`, tutte `static` e tutte di lettura —
`ProjectStatus`, `GetCurrentMap`, `DumpCell`, `FindPath`, `ValidateTacticalMap`. **Zero** creano, modificano,
compilano o salvano alcunché. Il §13 («*NON produrre soltanto istruzioni manuali*»), le liste MCP di `R2` e
`R4`, il §22 («*non chiedere intervento umano per lavoro eseguibile via Unreal MCP*») e l'intera catena di
chiusura del §23 poggiano su una capacità che non esiste su questa macchina.

🔴 **È il terzo giro sullo stesso terreno, e non lo sa.** Due kit gemelli sono stati consumati il
**2026-08-28** — [`2026-08-28-master-issue-reconciliation.md`](../../archive/src/handoff/2026-08-28-master-issue-reconciliation.md)
e [`2026-08-28-github-issues-roadmap-v01-v10.md`](../../archive/src/handoff/2026-08-28-github-issues-roadmap-v01-v10.md) —
il secondo dei quali ha aperto **31** issue (`#1557` → `#1587`). Questo kit non li nomina, e la sua §2
«*search before create*» si applica per prima a sé stesso.

⚠️ **Il suo punto cieco più costoso ha un numero: `#1408`.** `E48 · Il giocatore raggiunge il modello` è
l'owner più recente di ciò che il kit chiama `R2 — PLAYER UNDERSTANDS THE PLAN`, ed è **assente** dalla sua
lista di ancore. Chi eseguisse `R2` con le sole ancore del §5 aprirebbe un owner parallelo a `E48` —
esattamente ciò che il §27 vieta.

⚠️ **Non ha una data.** È l'unico fra i kit archiviati a non averla, né nel nome né in testa. Le sue
istruzioni di riverifica («*baseline recente*», «*ordine di taglio corrente*») non hanno un istante a cui
riferirsi, e fra le sue misure e la lettura possono passare giorni: il `G1` del DoD è stato **rosso per due
giorni** fra il 27 e il 29 agosto senza che una riga di documento cambiasse.

## Cosa è stato misurato

Ogni riga è stata verificata sul branch corrente. Chi rilegge riesegua i comandi: nessuno di questi numeri
va copiato.

| § | Ciò che il kit afferma | Misura | Esito |
|---|---|---|---|
| §4 | i dieci percorsi del preflight esistono | test di esistenza, **10/10** presenti | ✅ |
| §3 | Feature Registry e `parallel-batch.yaml` rimossi, non ricrearli | `git ls-files` → **0** file vivi, due sole copie in `docs/archive/` | ✅ (`D-181`) |
| §5 | 29 ancore: 10 epic + 19 issue | `gh issue view` una per una → **29/29** esistono, titoli coincidenti | ✅ |
| §5 | *(implicito)* le ancore sono lavoro vivo | **2 su 29 sono CLOSED**: `#78` intenti/certainty, `#287` personaggi sui centri hex | ⚠️ |
| §5 | «il parent `#14` può essere indietro» | `#14` aggiornata **2026-08-25**, cita 45 numeri; **4** epic `[EPIC v0.1]` aperte non compaiono: `#324`, `#934`, `#952`, `#1408` | ✅ premessa vera |
| §5 | «cerca live `E46`, `E47`» | esistono: `#934` *Frontend shell e ciclo di partita*, `#952` *Mini v0.1 Autobattle* | ✅ |
| §5 | *(assenza)* nessuna menzione di `E48` | `#1408` è aperta, è `[EPIC v0.1]`, ed è l'owner di `R2` | 🔴 lacuna |
| §10/R7 | «baseline recente `G1–G14`, ma riverifica» | il DoD dichiara `G1`…`G15`; `G15` è **ritirato** (`D-181`) e il numero non si riusa → **14** gate vivi | ✅ |
| §10/R4 | «se la label `READY_FOR_HUMAN_REVIEW` non esiste, non inventarla» | `gh label list` → **nessuna** label `review`/`human`/`ready`/`blocked` | ✅ e ora misurato: **non esiste** |
| §27 | «considerare replay = resimulation» è vietato | [ADR-0009](../../decisions/adr-0009-replay-logico-canonico.md) è esattamente questa separazione | ✅ canonico |
| §1 | scope v0.1: 2v2, offline vs bot, hex multilivello, 4 eroi; `4v4` fuori scope come formato di prodotto | coincide col pin di [`CLAUDE.md`](../../../CLAUDE.md) §3 | ✅ |
| §13 §10/R2 §10/R4 §22 §23 | Unreal MCP crea/modifica Blueprint e Widget, assegna classi, imposta proprietà, **compila**, **salva**, **lancia PIE**, cattura evidenza | `RTDevToolset.h`: **5** `UFUNCTION(meta = (AICallable))`, tutte `static`, tutte restituiscono un report. **0** scritture | 🔴 **falso** |

I comandi, per rieseguirli:

```bash
git fetch --prune origin
for n in 14 24 25 26 151 152 153 217 286 324 38 77 78 79 80 85 166 171 172 173 219 220 287 288 289 613 705 314 319; do
  printf '#%-4s ' "$n"; gh issue view "$n" --json state,title -q '.state + " | " + .title'
done
gh issue list --search '"[EPIC v0.1]" in:title' --state open --limit 100 --json number,title
gh issue view 14 --json body -q .body | grep -oE '#[0-9]{1,4}' | tr -d '#' | sort -u
gh label list --limit 100 --json name -q '.[].name' | grep -iE 'review|human|ready|blocked'
grep -nE 'UFUNCTION' Plugins/RTDeveloperTools/Source/RTDeveloperTools/Public/RTDevToolset.h
grep -oE '\bG[0-9]{1,2}\b' docs/roadmap/v0.1-definition-of-done.md | sort -uV
```

## Il difetto che conta: il §13 non è eseguibile

Il kit dedica tre capitoli — §13, §22, §23 — a una regola operativa sola: *se l'Editor è aperto, Claude non
scrive istruzioni manuali, esegue*. La catena del §23 è

```text
MCP implementation → Blueprint/Widget compiles → PIE functional verification → automation → human gate → DONE
```

Nessuno dei primi quattro anelli è raggiungibile. Il toolset RT è **facade di sola lettura** sul codice
autorevole: `FindPath` chiama `URTHexPathLibrary::FindPath`, `DumpCell` interroga `URTHexMapAsset`,
`ValidateTacticalMap` chiama `ValidateMap`. È una scelta dichiarata in [`CLAUDE.md`](../../../CLAUDE.md) §5
(«*nessuna scrittura*»), non una lacuna da colmare di corsa — e il kit la contraddice senza accorgersene.

**Perché conta più di un errore di dettaglio**: il §22 chiude con «*non chiedere intervento umano per lavoro
eseguibile via Unreal MCP*». Applicata alla superficie reale, quella riga non toglie lavoro all'autore: lo
sposta in un limbo dove nessuno lo rivendica. Le issue `EDITOR_MCP` del §7 resterebbero aperte senza owner
umano *e* senza esecutore automatico.

⚠️ **E la stessa riga non copre il caso peggiore**: `.mcp.json` punta a `127.0.0.1:8765`, una porta sola. In
una macchina con più cloni — che è il caso misurato di `D-222` — un ponte solo parla al clone che ha aperto
l'Editor per primo, che può non essere quello su cui stai lavorando.

## Il panel

### Karl Wiegers — qualità del requisito

❌ **CRITICO · il §9 dichiara l'idempotenza e non la rende verificabile.** «*Due esecuzioni consecutive senza
cambi devono produrre `Created = 0`*» è un criterio eccellente e non ha strumento. Il kit gemello del
2026-08-28 portava un **manifest JSON** accanto a sé; questo non porta niente. Senza un artefatto che
registri cosa è stato toccato e su quale `HEAD`, la seconda esecuzione non può distinguere «*non c'era nulla
da fare*» da «*non ho cercato*».
📝 **Raccomandazione**: il §20 «report per batch» diventa un file, non una stampa. Un `.json` con
`{issue, azione, evidenza, HEAD}` per riga rende il §9 falsificabile con un `diff`.

⚠️ **MAGGIORE · il §6 promette una tabella di audit con tredici colonne e non ne dà nessuna riga.** Il
vocabolario `Action` ha **nove** valori; la procedura che porta dall'evidenza al valore è scritta per **uno
solo** (`DONE`, nel riquadro «*issue closed + code exists + …*»). Gli altri otto si decidono a intuito.
📝 **Raccomandazione**: una riga di esempio compilata per almeno `CLOSE_DUPLICATE`, `REOPEN` e
`HUMAN_REVIEW`, con l'evidenza minima che ciascuno pretende.

⚠️ **MAGGIORE · le otto modalità del §7 non entrano nella tabella del §6.** Le colonne sono `Code`,
`Editor Asset`, `Automation`, `Human PIE`, `Packaged`: `EDITOR_MCP`, `DOCS` e `ART_ASSET` non hanno dove
essere scritte. Una classificazione che il formato di raccolta non può contenere si perde alla prima riga.

### Gojko Adzic — esempi ed eseguibilità

⚠️ **MAGGIORE · i gate di `R1`–`R6` sono frasi, non esempi.** «*Una partita completa termina con un esito
dichiarato*» (`R1`) e «*il giocatore può rispondere a: cosa farà questa unità?*» (`R2`) sono buoni criteri
umani senza protocollo di esecuzione — mentre il protocollo **esiste già**:
[`test-manuali-pie.md`](../../technical/test-manuali-pie.md) ha un subset `RELEASE-V01` di **17** voci
`PIE-*`, ed è la metà umana del gate `G9`. Il kit lo fa leggere nel §4 e poi non lo usa mai.
📝 **Raccomandazione**: ogni gate `R1`–`R6` cita gli ID `PIE-*` che lo dimostrano. Un gate che non nomina la
voce capace di falsificarlo non è un gate, è un auspicio.

✅ **Merito · il §17 è un albero decisionale eseguibile.** `candidate → semantic search → owner? → REUSE /
UPDATE / CREATE / DEFER` è la parte migliore del documento e andrebbe promossa davanti a tutto, accanto
al §2.

### Alistair Cockburn — attori e scopi

✅ **Merito · il §22 separa gli attori come si deve.** *Claude possiede audit, CRUD, docs, C++, test*;
*Aaron possiede product decision, UX judgement, visual sign-off*. È la distinzione che rende il kit
utilizzabile.

❌ **CRITICO · e l'ultima riga del §22 la rompe.** «*Non chiedere intervento umano per lavoro eseguibile via
Unreal MCP*» è condizionata a una capacità inesistente: dove il kit crede ci sia un esecutore automatico c'è
un buco.
📝 **Raccomandazione**: invertire il default. *Ogni lavoro d'Editor è umano finché un tool MCP nominato non
dimostra il contrario.* Il ponte è di lettura: serve a **preparare** e **verificare** la sessione umana, non
a sostituirla.

⚠️ **MAGGIORE · manca l'attore «le altre sessioni».** `D-222` misura più sessioni sulla stessa working
directory — 101 checkout e 6 sessioni in un giorno. Il kit scrive come se fosse solo: nessuna riga su
branch, su PR, sul fatto che una issue chiusa mentre un'altra sessione la sta lavorando è lavoro distrutto.

### Martin Fowler — responsabilità e confini

⚠️ **MAGGIORE · il documento fa tre cose e le chiama una.** (1) un audit di verità del tracker (`R0`, §6,
§18); (2) una roadmap di release (`R1`–`R7`, §11, §12); (3) un protocollo di esecuzione Editor (§13, §23).
Hanno cadenze, verificatori e proprietari diversi: il primo è ripetibile a costo quasi zero, il secondo
cambia solo per decisione d'autore, il terzo dipende da quale software gira sulla macchina. Tenuti insieme,
il terzo — l'unico rotto — trascina con sé i primi due.
📝 **Raccomandazione**: eseguire `R0` e il §17 **oggi**; congelare §13/§23 finché la superficie MCP non
cambia; trattare `R1`–`R7` come proposta di sequenza da confermare contro
[`roadmap-checkpoint.md`](../roadmap-checkpoint.md).

⚠️ **MINORE · §15 e §16 sono due template quasi identici.** Diciassette campi contro quattordici, con
`Current measured state` presente solo nel secondo — che è il campo che porta l'evidenza, e serve **di più**
all'epic che alla issue.

### Michael Nygard — modi di fallimento

❌ **CRITICO · il §25 STEP 3 autorizza sé stesso.** «*Esegui le mutazioni non ambigue*» rende il documento la
propria autorizzazione. Il guardrail di [`CLAUDE.md`](../../../CLAUDE.md) §7 dice il contrario, e il
precedente del 2026-08-28 lo conferma: il kit gemello è stato consolidato **dopo conferma d'autore**.
📝 **Raccomandazione**: STEP 3 si esegue su conferma esplicita, e la conferma nomina il **batch** (§19), non
il documento.

⚠️ **MAGGIORE · nessun rollback, e le operazioni non sono simmetriche.** Un `UPDATE` sbagliato si corregge;
una issue chiusa `not_planned` per errore lascia una notifica a chiunque la seguisse e una data che non torna
indietro. Il §8 non gradua il rischio fra i due.

⚠️ **MAGGIORE · il §25 STEP 3 prescrive `STOP → OPEN DECISION` senza dire come si numera una decisione.** È
lo stesso difetto del kit gemello, e il progetto ha già pagato **diciassette** collisioni di `D-nnn`. Il
protocollo vive in [`CLAUDE.md`](../../../CLAUDE.md) §7 — si legge dai **ref remoti**, non da `gh pr list`.

### Lisa Crispin — validazione

⚠️ **MAGGIORE · il §6 dice «*mai decidere lo stato da un solo indizio*» e il §21 chiede una colonna sola.**
«*Can Claude do it?* `YES / MOSTLY / PARTIAL / NO`» comprime in un valore ciò che il §7 ha appena scomposto
in otto modalità. Una issue `CODE + EDITOR_HUMAN_REVIEW` non è `MOSTLY`: è `YES` per metà e `NO` per
l'altra, e la seconda metà è quella che decide quando si chiude.

✅ **Merito · il §14 è la lista di stop più onesta del documento.** «*si legge bene?*», «*è troppo
affollato?*», «*la camera è comoda?*» sono i criteri che nessuna automazione può firmare, ed elencarli per
esteso vale più di qualunque label.

⚠️ **MINORE · `#78` e `#287` sono CLOSED e il §5 le presenta come le altre.** Chi pianifica `R2` attorno a
«`#78` intenti / certainty» pianifica attorno a lavoro già consegnato.

## Se lo si esegue: l'ordine che regge

1. ✅ **`R0` (tracking truth) — chiuso il 2026-08-30**: epic riconciliate
   ([*cosa è stato riconciliato*](#r0--cosa-è-stato-riconciliato)), le due sweep eseguite
   ([*orfane e duplicati*](#r0--le-due-sweep-orfane-e-duplicati)) e le riparazioni applicate — sette padri
   allineati, tre `LINK` resi seguibili. ⏳ Resta **una domanda di lettura, non di misura**: quali delle 36
   senza dichiarazione vogliono un parent.
2. **Prima di qualunque `CREATE`, leggere i due gemelli del 2026-08-28** e le **31** issue `#1557`–`#1587`
   che il secondo ha aperto. La §2 del kit lo impone; il kit non lo sa.
3. **Aggiungere `E48` (`#1408`) alle ancore di `R2`** prima di toccare la UX, e verificare cosa `E48`
   possiede già delle nove voci che elenca.
4. **Congelare §13, §22 (ultima riga) e §23** finché il ponte MCP resta di sola lettura. Le issue
   `EDITOR_MCP` sono `EDITOR_HUMAN_REVIEW` con preparazione assistita, non lavoro automatico.
5. **Legare i gate `R1`–`R6` alle voci `PIE-*` del subset `RELEASE-V01`**, che è il verificatore che il
   progetto ha già.
6. **Chiedere conferma d'autore prima dello STEP 3**, batch per batch, e registrare l'esito in un file — non
   in una stampa.

## `R0` — cosa è stato riconciliato

Eseguito il 2026-08-30 su conferma d'autore, **una sola mutazione**: il corpo di
[`#14`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/14).

| Cosa | Prima | Dopo |
|---|---|---|
| Epic figlie elencate | 21 | **25** — aggiunte `#324` (E23), `#934` (E46), `#952` (E47), `#1408` (E48) |
| Stato delle 21 già elencate | 9 caselle `[x]` | ✅ **nessuna correzione**: le nove `[x]` corrispondono esattamente alle nove `CLOSED`, verificate una per una |
| Totale di release nel corpo | `21 epic, 100 checkpoint` | **tolto**, sostituito dal rimando all'owner — come [`#962`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/962) ha fatto con le altre quattro copie |
| Criterio di chiusura | «i **15** gate `G1`–`G15`» | nota additiva: `G15` è **ritirato** da `D-181`, i gate vivi sono **14** |

🔴 **La scoperta di `R0` è la quinta copia del totale.** `#962` ha eliminato le copie di `21 epic, 100
checkpoint` il 2026-08-25 cercandole con `grep -rn "21 epic" docs/`. Il corpo di una issue GitHub **non è in
`docs/`**: quella copia è sopravvissuta allo sweep che la cercava, ferma al valore di prima del 2026-08-12.
Un totale che vive «in un posto solo» va verificato anche fuori dal filesystem che il comando percorre.

⚠️ **La divergenza lato roadmap è stata dichiarata, non sanata.** `E48` è assente dai documenti **owner** —
[`roadmap-v0.1.md`](../roadmap-v0.1.md) §3 e [`roadmap-checkpoint.md`](../roadmap-checkpoint.md) — e non ha
una serie `CP 48.x`: aggiungerla alla tabella richiederebbe un `CP` inventato. La §3 ora porta la nota che lo
dichiara, e il totale `24 epic, 119 checkpoint` **non è stato toccato**.

🔴 **E `E48` non era una scoperta di oggi.** La prima stesura di quella nota affermava che `E48` non compare
in nessun documento di `docs/roadmap/`: falso, perché il grep che l'aveva misurato era `docs/roadmap/*.md`,
che **non scende in `plans/`**. Rimisurato con `--exclude-dir=plans`: zero fuori dagli owner, ma **quattro**
referti la nominano, e
[`master-issue-reconciliation-spec-panel-2026-08-28.md`](master-issue-reconciliation-spec-panel-2026-08-28.md)
l'aveva già elencata due giorni prima fra le epic vive che un kit non citava. Era una **segnalazione non
raccolta**, e un glob non ricorsivo l'avrebbe fatta sembrare nuova una seconda volta.

## `R0` — le due sweep: orfane e duplicati

Eseguite il 2026-08-30 sul corpus completo: **286** issue aperte, **392** chiuse, **52** epic note.
Nessuna mutazione: sono misure.

### Il criterio, e come la prima stesura era sbagliata

La parentela in questo tracker si dichiara in **due** modi, e vanno letti entrambi: il figlio scrive
`**Epic**: #N` nella prima riga del corpo, oppure l'epic lo nomina nel proprio.

🔴 **La prima stesura leggeva solo le checklist `- [ ] #N` del padre, e produceva 72 «link a senso unico».**
Falso: **30 epic su 52 non hanno nemmeno una checkbox**, pur citando issue nel corpo — elencano i figli in
tabella o in prosa. Il criterio misurava il **formato**, non la parentela. Rifatto con «*l'epic nomina il
numero, in qualunque forma*», i casi scendono da 72 a **25**: due terzi erano un artefatto del criterio.

### Orfane, e la terza convenzione

🔴 **Le due «orfane con `CP` nel titolo» non erano orfane, e il criterio ha sbagliato una seconda volta.**
`#1324` e `#1206` dichiarano il proprio parent in una **riga di tabella** — `| Epic | LINK — … |` — che
nessuna delle due forme cercate poteva vedere. È lo stesso errore della checklist, un piano più in là:
misurare un **formato** invece del fatto. E `#1206` va oltre, dichiarando di **non** appartenere a nessuna
epic: «*`#214` (E19) e la feature `RT-FEAT-TIMEBANK` sono i soggetti nominati, ma la correzione non
appartiene a nessuna delle due*». Collegarle sarebbe stato l'errore opposto — e la richiesta di collegarle
nasceva dalla mia misura, non dal tracker.

| Categoria | Quante |
|---|--:|
| Flaggate dal criterio a intestazione (`**Epic**`/`**Parent**` in testa, o il padre che le nomina) | **46** |
| — di queste, dichiarano l'epic in **riga di tabella** | **10**: sei `LINK`, quattro `N/A` esplicito |
| — restano senza dichiarazione, in nessuna delle tre forme | **36** |
| A senso unico, col parent **immediato** (`**Parent**` vince su `**Epic**`) | **21** su 7 padri — erano **25** prima di questa correzione |

✅ **Riparato il 2026-08-30, e il difetto vero era un altro.** Delle sei righe `LINK`, **tre nominavano
l'epic per etichetta senza il numero** — e un'etichetta non è un link, perché nessuno la può seguire.
Aggiunto il numero a `#1330` (→ `#934`), `#1324` (→ `#22`) e `#1317` (→ `#23`); `#1119`, `#1132` e `#1206` lo
avevano già.

✅ **Sette padri allineati**: a ciascuno una nota datata che elenca i figli che lo dichiarano e che il suo
corpo non nominava — **21** in tutto. `#151` passa da un corpo che non nominava **nessuna** delle proprie
figlie a uno che le elenca tutte.

```text
#151  E13 Conoscenza parziale    11: #159 #160 #1466 #1496 #1497 #1498 #1499 #1500 #1525 #1535 #1715
#1105 Tactical Designer           3: #1678 #1682 #1683
#153  E15 Showcase «Il Relè»      3: #170 #171 #1060
#1678 DevSandbox Launcher         1: #1705   ← non è un'epic: è una issue che fa da padre
#25   E11 HUD, log e debug        1: #924
#952  E47 Mini v0.1 Autobattle    1: #1671
#775  E42 Dedicated server        1: #1621
```

⚠️ **Le note sono elenchi, non checklist**: dichiarano il legame, non lo stato né l'ordine. ⛔ `#934` (E46)
**esce** dalla lista rispetto alla misura precedente: `#938` e `#940` dichiarano un `**Parent**` diverso, che
le nomina — il criterio sbagliato le aveva attribuite a lui.

⚠️ **Fra le 42 senza `CP` ce ne sono due che nessuna epic nomina e che sono gate di release**: `#1663` (gli
eroi non hanno animazioni nel pacchetto) e `#1665` (la board è nera nel pacchetto). Toccano `G12`/`G13` e non
hanno un owner che li rivendichi. ⛔ **Altre sei sono `[DESIGN]`, e una `[DOCS/DESIGN]`**: sono decisioni, e
il loro owner è [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), non un'epic — cercarvi un parent sarebbe
l'errore opposto.

### Duplicati: zero

| Criterio | Esito |
|---|---|
| Titolo identico fra due aperte | **0** |
| Titolo identico fra un'aperta e una chiusa | **0** |
| Sovrapposizione lessicale (Jaccard sui titoli, stopword italiane rimosse) | **40 755** coppie confrontate, massimo osservato **0.429** — e le prime dieci sono tutte epic-contro-propria-figlia o release diverse |
| Stesso identificatore `CP` | **5** collisioni, verificate una per una: **nessuna è un duplicato** |

Le cinque collisioni `CP`, e perché nessuna si chiude:

- **`CP 11.8` → `#705` `#1614` `#1615`**: ✅ **non c'era niente da sistemare, ed è la scoperta migliore della
  sweep.** `#1614` e `#1615` dichiarano entrambe `**Parent**: #705` e scrivono in testa al corpo «*Non è un
  nuovo checkpoint: CP 11.8 resta `#705`*»; e `#705` le nomina a sua volta in un blocco dedicato. La
  gerarchia è dichiarata **tre volte, da entrambi i lati**. La «collisione» era un artefatto del criterio
  lessicale: tre titoli portano `CP 11.8` proprio perché due dicono di appartenervi. `#14`, che cita
  `CP 11.8 (#705)`, era **già corretta**.
- **`CP 19.3` → `#1124` `#1206`**: `#1206` non duplica `#1124`, parla **della** mis-attribuzione di `CP 19.3`
  in quattro documenti. Chiuderla come duplicato cancellerebbe proprio il rilievo.
- **`CP 8.3` → `#1324` aperta, `#66` `#282` chiuse**: `#1324` è il **follow-up** che misura che il lavoro di
  `CP 8.3` è incompleto (`Status.Electrified` è un tag inerte). È la forma che il §8 del kit prescrive —
  follow-up, non `REOPEN`.
- **`CP 11.7` → `#613` aperta, `#1545` chiusa**: `#1545` era una sotto-parte (`IconImage` su
  `WBP_RT_ActionSlot`), già consegnata.
- **`CP 13.4` → `#159` aperta, `#295` chiusa**: `#295` era un rilievo su `CP 13.4`, chiuso.

⛔ **Il limite di questa sweep, e non è piccolo: trova duplicati *lessicali*.** La §2 del kit chiede «*cerca
per concetto*», e nessuno di questi comandi lo fa. La prova è la coppia più vicina che **non** è un
duplicato: `#1166` e `#853` (0.38) parlano entrambe di *nomi legacy nei pixel delle immagini della Wiki*, e
sono **complementari** — il titolo di `#1166` dice «*fuori dal percorso Player*», che è esattamente ciò che
`#853` copre. Termini quasi identici, insiemi disgiunti. Un criterio lessicale non distingue i due casi, e
l'unico modo di chiudere questa domanda è leggerle.

## Cosa questa revisione non ha fatto

- **Nessun `CREATE`, `CLOSE` o `REOPEN`**: **11** mutazioni in tutto, tutte additive e tutte su corpi di
  issue — `#14`, sette note di riconciliazione, tre numeri d'epic. Nessun commento, nessuna label, nessuna
  milestone, nessuna issue aperta o chiusa.
- **Non ha eseguito `R1`–`R7`**, né il §17 sulle candidate UX, né alcun lavoro d'Editor.
- **Non ha dato un parent alle 36 senza dichiarazione**: molte non ne vogliono uno, e distinguerle richiede
  di leggerle. Il criterio dice solo che nessuna delle tre forme le collega.
- **Non ha verificato le 19 issue operative del §5 contro il codice**: ha verificato che esistano e che i
  titoli coincidano con ciò che il kit dice.

## La lezione di questo giro, che vale più dei numeri

Tre criteri meccanici, **due sbagliati nello stesso modo**: misuravano il *formato* di una dichiarazione
invece del *fatto* dichiarato. La checklist `- [ ] #N` ignorava 30 epic su 52; l'intestazione `**Epic**: #N`
ignorava dieci issue che lo dicono in tabella; e il titolo con `CP 11.8` scambiava per collisione tre issue
che dichiarano di appartenersi. Ogni volta il numero sbagliato era **più grande** del vero — 72 contro 21,
46 contro 36, una collisione contro zero — e ogni volta la correzione è arrivata leggendo il contenuto, non
raffinando la regexp. ⛔ **Un criterio che non è stato validato su un campione letto a mano non è una
misura**: è una proposta di misura, e va dichiarata così finché qualcuno non la controlla.
- **Non ha interrogato il ponte MCP a runtime**: la superficie è stata misurata sul **sorgente** del plugin.
  Se l'Editor fosse aperto, `tools/list` direbbe la stessa cosa — ma questa è una deduzione dal codice, non
  una chiamata.
- **Non ha misurato la suite**: `scripts/rt-suite.ps1` non è stato eseguito. Nessuna affermazione qui dipende
  da un esito di test.
