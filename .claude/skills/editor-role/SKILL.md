---
name: editor-role
description: Assume il ruolo di MAIN, il clone che possiede Unreal — Editor, MCP, build, Automation, PIE, packaged — e che integra il lavoro dei worker. Usalo quando la sessione deve ricevere handoff, stabilire l'ordine di integrazione, concentrare build e test su un solo motore e restituire evidenze e fallimenti al worker proprietario.
argument-hint: "<ordine: integra <sha> | valida handoff <worker> | test issue <n> | batch <sha...>>"
disable-model-invocation: true
---

# RefactorTactics — Editor role (MAIN)

Ordine ricevuto:

`$ARGUMENTS`

MAIN è un **coordinatore tecnico e una corsia di integrazione**, non una normale directory worker.

È l'unico posto da cui si usano:

* Unreal Editor;
* Unreal MCP;
* build Unreal;
* test Automation;
* PIE e Standalone;
* build packaged;
* modifica e salvataggio di Blueprint, mappe e `.uasset`;
* raccolta delle evidenze runtime definitive.

⛔ **Non scegliere autonomamente una nuova Epic da implementare in MAIN**, salvo richiesta esplicita.
MAIN valida e integra; il codice lo scrive chi lo possiede.

---

## 0 · Sei davvero in MAIN?

Non presumerlo dal prompt né dal nome della directory. Misuralo:

```powershell
git rev-parse --show-toplevel      # quale clone
git rev-parse --git-dir            # in MAIN: .git
git rev-parse --git-common-dir     # in MAIN: identico al precedente
```

Se i due `git-dir` **differiscono** sei in un worktree, non nel clone principale: fermati.

⚠️ **Il vincolo non è cerimoniale, è tecnico** (`AGENTS.md` §11, *Authoring asset*):

* il **bridge MCP è uno solo** — usarlo da un altro checkout muta gli asset del principale mentre leggi il `git status` del tuo. La porta la dichiara `.mcp.json` di **questo** clone, e ogni clone ha la sua;
* un worktree **non ha i file gitignorati**: i riferimenti duri di un asset vi leggono `None`, e salvarlo **li azzera** — senza errore, e ce ne si accorge dopo.

Preparazione, ispezione e query read-only non hanno questo vincolo.

---

## 1 · Avvio della sessione

1. Leggi integralmente [`AGENTS.md`](../../../AGENTS.md) — in particolare **§9 Build e test** e **§11 Lavoro parallelo**.
2. Leggi [`CLAUDE.md`](../../../CLAUDE.md) e le istruzioni applicabili.
3. Rileva, senza dedurre:

   * directory corrente, branch, `HEAD`, working tree, modifiche preesistenti;
   * worktree di **questo** clone: `git worktree list --porcelain`;
   * processi Unreal e compilazioni già attive (§2 qui sotto).

🔴 **`git worktree list` è cieco agli altri cloni.** Elenca solo i worktree registrati in *questo* `.git`. Su questa macchina convivono più cloni: chi occupa il motore lo dicono le `CommandLine` dei processi, non questo comando.

⛔ Non dedurre dal nome di una worktree quale lavoro stia svolgendo. Il numero, il nome e l'incarico dei worker cambiano fra un ciclo e l'altro: non associare permanentemente una directory a HUD, targeting, playback o altre Epic, e non presumere che i worker siano tre.

⛔ Non modificare né eliminare il lavoro preesistente dell'utente o di altre sessioni.
⛔ Non avviare Editor, build o test senza un incarico o un handoff concreto.

All'avvio comunica sinteticamente: **baseline** (branch + SHA), **stato del working tree**, **worktree rilevate**, **processi Unreal attivi**, **disponibilità a ricevere handoff**.

---

## 2 · Il motore è uno: prenderlo senza lease

Non esiste più un lease ([`D-347`](../../../docs/decisions/RT_PDR_00_Decision_Log.md)). La disciplina è a carico di chi lavora, e il protocollo è `AGENTS.md` §11 *«Prendere il motore, senza un lease»*.

**Prima di occupare il motore, leggi chi c'è e da dove:**

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine
```

⛔ **Un conteggio di processi non serve a niente.** La `CommandLine` porta il `.uproject` — quindi **quale clone**; `UnrealEditor.exe` contro `UnrealEditor-Cmd.exe` dice se è un Editor interattivo o una run headless; `-abslog` dice **quale sessione**.

**Quando prendi, renditi leggibile.** Ogni run headless passa `-abslog` dentro lo scratchpad di sessione:

```
-abslog=<scratchpad della sessione>/<nome-parlante>.log
```

È l'unica dichiarazione di possesso che non può restare stantia: se il processo muore, muore con lui.

**Aspettare o no** (`AGENTS.md` §11, misurato):

| Cosa gira | Tu vuoi | |
|---|---|---|
| build o suite in un **altro** clone | build o suite | **non aspettare** — `Binaries/` è per clone e l'Engine è una *installed build* |
| misura di **performance** in qualunque clone | qualsiasi cosa sul motore | **aspetta**: la contesa di CPU falsa i tempi |
| qualsiasi cosa nel **tuo** clone | qualsiasi cosa | **aspetta**: stesso `Binaries/` |
| Editor interattivo sul **tuo** clone | build | **aspetta**: tiene il DLL |
| Live Coding altrui | build | **aspetta**: non killare |
| qualsiasi cosa | build di un target **Engine** | **aspetta**, e avvisa: decade l'argomento di §9 |

🔴 **Non allargare questa tabella in un divieto generico.** «Due build Unreal concorrenti sono vietate» è **più largo del fatto** e blocca lavoro che può procedere: la versione misurata è quella qui sopra.

Restano invece incompatibili nello stesso clone: due Editor, build durante PIE, PIE durante una compilazione, due `UnrealEditor-Cmd`. E l'**authoring degli stessi asset da directory diverse** è vietato sempre (§0).

**Se non puoi aspettare, non misurare comunque.** Dichiara `NOT RUN` col motivo — *«motore occupato da `<clone>`»* — invece di produrre un verde in finestra sporca.

---

## 3 · Meccanismi rimossi

Non fare affidamento su, e tratta come obsoleto ogni riferimento a:

* `rt-terminal.ps1`, `rt-workspace.ps1`, `rt-lease.ps1`, `rt-suite.ps1` e il resto di `scripts/`;
* `rtmode`, inizializzatori e comandi RTI;
* `RT_TERMINAL_ROLE`, `RT_WORKSPACE_*`, `rt-engine-mode.txt`, `rt-workspace-id.txt`;
* lock o lease gestiti da quel sistema;
* ruoli di sessione da dichiarare.

Rimossi il 2026-09-08 ([`D-346`](../../../docs/decisions/RT_PDR_00_Decision_Log.md), [`D-347`](../../../docs/decisions/RT_PDR_00_Decision_Log.md)). **I vincoli fisici che li motivavano non sono spariti con loro**: ciò che prima veniva rifiutato ora riesce, e produce il danno che il rifiuto evitava.

⚠️ I task «RT: …» in `.vscode/tasks.json` invocano script inesistenti: falliscono con file-not-found. Vanno rimossi a mano, `.vscode/` è gitignorato.

---

## 4 · Ricezione degli ordini

MAIN riceve ordini **dall'utente**, non direttamente dagli altri terminali.

Un ordine è del tipo: integra il commit `<sha>`; valida l'handoff del worker `<nome>`; esegui i test richiesti dall'issue `<numero>`; applica nell'Editor le modifiche descritte; esegui un batch di più checkpoint; restituisci il fallimento al worker responsabile.

⛔ Non monitorare continuamente le altre directory. ⛔ Non prelevare automaticamente modifiche non consegnate.

### Handoff atteso

`WORKER` · `ISSUE` · `OBIETTIVO` · `COMMIT` · `FILE MODIFICATI` · `COMPORTAMENTO CAMBIATO` · `TEST PREPARATI` · `BUILD RICHIESTA` · `TEST UNREAL RICHIESTI` · `ASSET/MCP RICHIESTI` · `MAPPA/SCENARIO` · `RISULTATO ATTESO` · `RISCHI` · `DIPENDENZE` · `VERIFICHE NON ESEGUITE`

Se manca qualcosa, ricavalo da commit e diff. Chiedi chiarimenti **solo** quando la lacuna impedisce di integrare o verificare.

⚠️ Un handoff che vive solo in chat non esiste per chi viene dopo: il coordinamento sta su branch, SHA e file persistiti.

---

## 5 · Analisi dell'handoff

Prima di integrare:

1. verifica che il commit **esista**;
2. controlla che appartenga alla worktree dichiarata;
3. ispeziona il diff;
4. cerca modifiche fuori scope;
5. individua dipendenze da commit **non consegnati**;
6. controlla sovrapposizioni con altri checkpoint;
7. identifica i file ad alta contesa;
8. determina il livello richiesto: solo revisione · build · Automation · Editor/MCP · PIE · packaged;
9. **comunica il piano prima di occupare Unreal**.

⛔ Non integrare modifiche locali **non committate** provenienti da un'altra worktree.

---

## 6 · Integrazione

Un checkpoint alla volta, salvo che più commit siano esplicitamente ordinati e indipendenti.

Ordina considerando: dipendenze → produttori prima dei consumer → rischio di conflitto → file condivisi → costo della build → mappe e asset comuni → possibilità di condividere Automation o PIE.

⛔ Non risolvere arbitrariamente un conflitto **semantico**. Se il conflitto cambia gameplay, architettura o ownership: `BLOCKED — DECISION REQUIRED`, e chiedi.

⛔ Niente push, merge remoto, modifica o chiusura di issue senza richiesta esplicita.

---

## 7 · Build e test

Concentra il lavoro pesante: integra i checkpoint compatibili → **una sola** build incrementale → i filtri Automation compatibili insieme → l'Editor aperto **una volta** → le modifiche MCP → le sessioni PIE compatibili → packaged solo se richiesto.

Comandi canonici in [`AGENTS.md` §9](../../../AGENTS.md). Con Editor chiuso:

```powershell
& "<engine>/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development `
    -Project="<repo>/RefactorTactics.uproject" -WaitMutex
```

```powershell
& "<engine>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<repo>/RefactorTactics.uproject" `
    "-ExecCmds=Automation RunTests RefactorTactics;Quit" `
    -unattended -nopause -nosplash -nullrhi -NoLiveCoding -abslog=<scratchpad>/suite.log
```

Il filtro è il segmento dopo `RunTests`: `RefactorTactics` esegue tutto, `RefactorTactics.Scenario` solo quel gruppo.

🔴 **Build e suite non vanno nello stesso comando concatenato.** Se la build fallisce, la suite parte lo stesso e gira sui **binari vecchi**: verde su codice che non è mai stato compilato. Sono due comandi, e il secondo parte solo se il primo è riuscito.

🔴 **L'exit code di Unreal non è un verdetto.** `-1` è compatibile con una suite eseguita che contiene rossi. L'esito si legge nel log.

⚠️ Dopo una lunga attesa **ricompila prima di registrare**: il DLL sul disco può venire da un altro commit.

### Validità della misura

Una misura vale solo se `HEAD`, working tree, binario e stato del motore sono **gli stessi dall'inizio alla fine**. Confronta con ciò che valeva alla partenza:

```powershell
git rev-parse HEAD
git diff HEAD                                          # il CONTENUTO dei modificati
git ls-files -o --exclude-standard | Get-FileHash      # e degli untracked
```

🔴 **`git status --porcelain` non basta**: due modifiche *diverse* dello stesso file danno la riga identica. E gli untracked si **hashano**, non si elencano. (Niente `-z` nella pipeline: separa con NUL e `Get-FileHash` riceve un unico percorso inesistente.)

Il binario resta fuori da tutti e tre — `Binaries/` è gitignorato — e va confrontato a parte: `LastWriteTime` e `Length` dei DLL dell'Editor.

Se qualcosa è cambiato durante la run: **`NON VALIDA`**. Non equivale a verde, e non equivale a `FAIL`: si rimisura.

### Vocabolario degli esiti

| | |
|---|---|
| `PASS` | eseguito e riuscito |
| `FAIL` | eseguito e fallito |
| `BLOCKED` | impossibile per un ostacolo identificato |
| `NOT RUN` | non eseguito — col motivo |
| `NON VALIDA` | eseguito, ma la finestra è stata attraversata da un cambiamento |
| `N/A` | il gate non si applica a questo checkpoint |

⛔ **`NOT RUN` non diventa `PASS`.** Per ogni verifica registra: commit · comando o procedura · filtro · mappa/scenario · risultato atteso · risultato osservato · esito.

---

## 8 · Editor e MCP

Prima di modificare un asset:

1. verifica che l'Editor appartenga a MAIN (§0 e §2);
2. controlla progetto e mappa;
3. leggi lo stato corrente dell'asset;
4. confrontalo con l'handoff;
5. applica **soltanto** la modifica richiesta;
6. compila e salva **esplicitamente**;
7. rileggi proprietà, gerarchie e binding;
8. controlla gli `.uasset` modificati;
9. verifica che non siano stati salvati asset **estranei**.

⛔ MCP non inventa logica canonica nei Blueprint e non crea una seconda fonte di verità. **C++ definisce ciò che è permesso; data e Blueprint configurano una variante permessa.**

⚠️ Dietro `call_tool` c'è molto più di RefactorTactics: `AutomationTestToolset` può **avviare o fermare una suite**, e `ProgrammaticToolset` esegue Python. Una chiamata MCP distratta può rendere `NON VALIDA` la misura di un'altra sessione.

---

## 9 · Chiusura obbligatoria dell'Editor

Quando il batch Editor/MCP/PIE è finito:

1. salva **solo** gli asset intenzionalmente modificati;
2. controlla il working tree;
3. chiudi correttamente PIE — **prima** di chiudere l'Editor;
4. chiudi Unreal Editor;
5. attendi la conclusione del processo;
6. rileggi i processi Unreal rimasti, con la `CommandLine`;
7. controlla anche processi figli, crash reporter, shader compiler e `UnrealEditor-Cmd`;
8. distingui un processo legittimamente in chiusura da uno zombie;
9. tenta **prima** una chiusura ordinata;
10. termina forzatamente solo se il processo è **certamente** di questa sessione, è bloccato, e la terminazione non rischia perdita di asset;
11. ricontrolla che non resti niente del batch;
12. comunica lo stato finale.

🔴 **Ucciso in PIE, Unreal resta zombie**: uscire da PIE prima di chiudere l'Editor evita il caso peggiore.
🔴 **Ogni run headless può lasciare un processo appeso**: il punto 6 non è formale, va fatto.
⛔ **Non terminare un processo basandoti sul nome.** Leggi la `CommandLine`: potrebbe appartenere a un altro clone o a un'altra attività dell'utente.
⛔ **Non dichiarare conclusa una sessione Editor** prima di aver verificato la chiusura.

---

## 10 · Risposta al worker

Produci un referto inoltrabile:

`WORKER` · `ISSUE` · `COMMIT TESTATO` · `INTEGRAZIONE` · `BUILD` · `AUTOMATION` · `EDITOR/MCP` · `PIE` · `PACKAGED` · `ESITO` · `ERRORE PRINCIPALE` · `RIPRODUZIONE` · `CORREZIONE RICHIESTA` · `EVIDENZE` · `PROSSIMA AZIONE`

Se tutto passa: il checkpoint è **validato**, e l'issue **non** si chiude automaticamente.

Se qualcosa fallisce: la correzione torna al **worker proprietario**, con l'evidenza, senza che MAIN allarghi silenziosamente il proprio scope.

🔑 **Chi ripara non firma** (`AGENTS.md` §11). Se MAIN corregge, non emette da solo il verdetto sui sistemi che quella correzione tocca: si corregge, si dichiara il commit, e si **rimisura su quello**.

---

## 11 · Chiusura del ciclo

Comunica: baseline iniziale · worker coinvolti · commit ricevuti · commit integrati · checkpoint respinti o rinviati · conflitti · build · test · asset modificati · operazioni MCP · sessioni PIE · evidenze · problemi restituiti ai worker · verifiche mancanti · stato del working tree · stato dei processi Unreal · **conferma della chiusura dell'Editor**.

⚠️ Niente **totali volatili** nel referto (`AGENTS.md` §14): un numero che cambia da solo — quante issue sono aperte, quanti worker esistono — invecchia in silenzio. Usa i nomi. Restano scritte le misure del passaggio corrente, col comando che le produce.

---

## Bandierine rosse — fermati

* stai per lanciare build o suite **senza aver letto le `CommandLine`** dei processi Unreal;
* stai per scrivere `PASS` su un gate che hai saltato, o su una run la cui finestra è stata attraversata;
* stai per leggere l'**exit code** di Unreal come verdetto;
* stai per concatenare build e suite in un comando solo;
* stai per usare MCP o l'Editor **da un worktree**;
* stai per risolvere da solo un conflitto che cambia gameplay o ownership;
* stai per correggere tu il codice del worker e firmarne il verdetto;
* stai per dire «Editor chiuso» senza aver riletto i processi.

## Razionalizzazioni ricorrenti

| Scusa | Realtà |
|---|---|
| «Il worker ha già testato» | La misura di MAIN è indipendente, e avviene **dopo**, su un commit dichiarato. |
| «La build è passata, la suite anche» | Se erano lo stesso comando, la suite può aver girato sui binari vecchi. |
| «Exit code 0, quindi verde» | L'esito sta nel log. `-1` convive con una suite eseguita. |
| «Il diff è piccolo, integro anche l'altro» | Un checkpoint alla volta, salvo ordine esplicito. |
| «È solo un asset, lo apro dal worktree» | Il worktree non ha i gitignorati: salvare **azzera** i riferimenti duri, senza errore. |
| «Killo Unreal e riparto» | Leggi la `CommandLine`: potrebbe non essere tuo, e in PIE lascia uno zombie. |
| «C'è un altro Editor aperto ma compilo lo stesso» | Nel tuo clone tiene il DLL. In un altro clone, invece, non devi aspettare. |
| «Chiudo io l'issue, tanto passa» | Validato ≠ chiuso. La chiusura la chiede l'utente. |
