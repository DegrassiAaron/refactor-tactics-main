# Prompt per Claude Code — consolidamento RT3, MCP e authoring asset solo da MAIN

Lavora dalla root del repository **RefactorTactics** in una sessione RT3 con ruolo `DEV`.

## Missione

Analizza criticamente e poi consolida, senza duplicazioni o policy divergenti:

- `AGENTS.md`;
- `CLAUDE.md`;
- `docs/rt-three-terminals/prompts/RT3_CONTRACT.md`;
- `docs/rt-three-terminals/prompts/TERMINAL_EDITOR.md`;
- gli eventuali prompt `WAVE_EDITOR.md`, `TERMINAL_VALIDATION.md` e `WAVE_VALIDATION.md` solo dove necessario per mantenere coerenza;
- `docs/rt-three-terminals/README.md` e `APPLY.md`;
- installer, task VS Code e script RT3;
- configurazione e wrapper/proxy delle MCP, in particolare Unreal/Epic MCP.

L'obiettivo è ottenere un solo modello operativo coerente:

1. ogni directory può aprire terminali `DEV`, `EDITOR` e `VALIDATION`;
2. il ruolo appartiene alla singola sessione, mai alla directory;
3. tutte le directory usano `WorkspaceRole=ALL`;
4. ogni directory possiede però un'identità di workspace distinta;
5. Unreal è una risorsa macchina esclusiva acquisita just-in-time;
6. avviare Claude con ruolo `EDITOR` o `VALIDATION` non acquisisce Unreal;
7. tutte le MCP sono classificate per capacità e side effect;
8. la creazione o modifica di asset tramite Unreal MCP è consentita soltanto a una sessione `EDITOR` eseguita dal workspace identificato come `MAIN`;
9. il workspace MAIN non equivale al branch Git `main`: l'authoring deve avvenire su un branch task-specifico, salvo esplicita istruzione contraria;
10. policy testuali e strumenti devono applicare la stessa regola fail-closed.

Non limitarti a riscrivere documentazione. Individua e correggi gli enforcement point reali. Se il trasporto MCP permette di invocare direttamente tool mutanti aggirando il wrapper, considera la policy **non implementata** finché il bypass non è rimosso, disabilitato o chiaramente bloccato.

---

## Fase 0 — preflight e critica obbligatoria

Prima di modificare file:

1. leggi integralmente `AGENTS.md`, `CLAUDE.md` e tutti i file attuali di `docs/rt-three-terminals/`;
2. individua tutte le configurazioni MCP del repository, dell'utente e del workspace che influenzano Claude Code, senza mostrare segreti;
3. individua tutti gli script/task che possono avviare Unreal, PIE, commandlet, Automation, cook, package o una MCP editor-facing;
4. misura e riporta:
   - directory canonica;
   - branch corrente;
   - `HEAD`;
   - `origin/main` dopo fetch sicuro;
   - stato del working tree;
   - file locali già modificati;
5. dichiara il write-set previsto;
6. cerca regole duplicate o contrastanti con `rg`;
7. presenta una critica sintetica organizzata come:
   - problema;
   - rischio concreto;
   - enforcement point;
   - correzione proposta;
   - test che dimostra la correzione.

La critica deve coprire almeno questi rischi:

- directory confusa con ruolo;
- workspace `MAIN` confuso con branch `main`;
- autorizzazione basata soltanto sul nome della cartella;
- `Start Claude` che acquisisce il lease troppo presto;
- mode locale `.vscode` usato erroneamente come prova di ownership globale;
- due clone che vedono lock differenti;
- owner metadata stale o scritti con race;
- rilascio del lease mentre Unreal o un processo figlio è ancora vivo;
- processo Unreal già aperto ma senza owner verificabile;
- MCP collegata all'Editor, progetto, PID o endpoint sbagliato;
- chiamata MCP mutante effettuabile senza passare dal wrapper;
- classificazione incompleta dei tool MCP nuovi o sconosciuti;
- asset binari modificati da più task o writer;
- VALIDATION che corregge un finding e poi auto-approva;
- log senza task ID, con informazioni insufficienti oppure contenente segreti;
- TOCTOU: contesto verificato nel preflight ma cambiato prima della chiamata MCP;
- incompatibilità Windows PowerShell 5.1 e path contenenti spazi;
- installer non atomico o distruttivo verso configurazioni locali;
- falsa dichiarazione `PASS` per test non eseguiti su Windows o senza Unreal.

Se trovi una decisione architetturale realmente bloccante, fermati dopo la critica con `BLOCKED` e una domanda precisa. Non inventare l'interfaccia di una MCP che non hai ispezionato.

---

## Autorità documentale da ottenere

Evita di copiare lo stesso contratto in molti file.

- `AGENTS.md` possiede i principi normativi condivisi: ruoli, workspace, risorse, asset, indipendenza della validation e fail-closed.
- `RT3_CONTRACT.md` possiede schema operativo, preflight, capability matrix, lease, handoff, log e verdetti.
- `CLAUDE.md` è un overlay Claude-specifico: rileva il contesto, carica un solo ruolo e applica il contratto senza riscriverlo.
- `TERMINAL_EDITOR.md` possiede i limiti operativi del ruolo EDITOR e rimanda al contratto.
- `WAVE_EDITOR.md` descrive una wave, non ridefinisce ruolo, workspace o lock.
- README/APPLY spiegano installazione, uso quotidiano, migrazione e rollback.
- task, configurazioni MCP e wrapper applicano concretamente la policy.

Ogni regola duplicata deve essere sostituita da un riferimento all'owner documentale, quando possibile.

---

## Modello obbligatorio: ruolo, workspace e capability

### Ruolo della sessione

Ogni sessione dichiara esattamente uno fra:

```text
DEV | EDITOR | VALIDATION
```

Il ruolo non si deduce dal nome della directory. In tutte le tre directory devono poter nascere N terminali di ciascun ruolo, compatibilmente con write-set e risorse condivise.

`WorkspaceRole` deve essere `ALL` per il modello consigliato e per l'installazione nelle tre directory. Mantieni eventuale compatibilità con valori storici soltanto se non crea un secondo modello operativo; documentali come legacy/deprecati.

### Identità del workspace

Introduci o consolida un'identità distinta dal ruolo, con valori almeno:

```text
MAIN
DEV
TECHNICAL_DESIGNER
```

Usa un nome coerente, per esempio `RT_WORKSPACE_ID`, e un marker locale esplicito, per esempio:

```text
.vscode/rt-workspace-id.txt
```

Non autorizzare `MAIN` soltanto perché il path contiene la parola “Main”. Registra la root canonica tramite installer e coordinamento macchina sotto:

```text
%LOCALAPPDATA%\RefactorTactics\RT3\
```

La registrazione deve:

- normalizzare il path assoluto;
- collegarlo al `.uproject` atteso;
- rifiutare identità mancanti o ambigue;
- evitare due workspace `MAIN` simultanei per lo stesso progetto, oppure richiedere una riconfigurazione esplicita e sicura;
- gestire lo spostamento della directory senza promuovere automaticamente un'altra cartella a MAIN;
- non trattare il marker locale modificabile come unico confine di sicurezza operativa.

Non versionare preferenze macchina-specifiche se il repository attuale le ignora intenzionalmente. Riconcilia con `.gitignore` e installer.

### Capability matrix minima

Implementa e documenta almeno queste classi:

| Capability | Requisiti |
|---|---|
| `MCP_READ_ONLY` | ruolo compatibile; nessun side effect |
| `WORKTREE_WRITE` | task/issue e write-set dichiarati |
| `EXTERNAL_WRITE` | task, target remoto, ownership e log; idempotenza dove applicabile |
| `UNREAL_USE` | lease macchina vivo e contesto Unreal verificato |
| `MCP_ASSET_READ` | contesto progetto verificato; lease se richiede Editor vivo |
| `MCP_ASSET_WRITE` | `EDITOR` + `WorkspaceId=MAIN` + branch task-specifico + task ID + write-set asset + lease Unreal + binding MCP verificato |
| `VALIDATION_SIGNOFF` | sessione `VALIDATION` indipendente, input immutato ed evidenza sufficiente |

Un tool MCP sconosciuto o non classificato deve essere negato per default quando potrebbe produrre side effect.

---

## Policy inderogabile: asset MCP soltanto da MAIN

Una chiamata che crea, modifica, rinomina, sposta, cancella, importa, reimporta o salva `.uasset`, `.umap`, Blueprint, Data Asset, Widget, Material/MI o altri asset Unreal è autorizzata soltanto se tutte le condizioni sono vere:

```text
RT_TERMINAL_ROLE == EDITOR
RT_WORKSPACE_ID == MAIN
workspace root == MAIN registrato per questo progetto
current branch == branch della task e current branch != main
RT_TASK_ID presente e valido
asset write-set dichiarato e non sovrapposto
lease Unreal globale vivo e posseduto dalla sessione
uproject, Editor PID ed endpoint MCP corrispondono al lease
nessun contesto è cambiato fra check e invocazione
```

Se una condizione fallisce, usa un errore stabile e diagnosticabile, per esempio:

```text
ASSET_WRITE_WRONG_WORKSPACE
ASSET_WRITE_ROLE_DENIED
TASK_CONTEXT_MISSING
PROTECTED_BRANCH_DENIED
ENGINE_LEASE_REQUIRED
MCP_CONTEXT_MISMATCH
ASSET_WRITESET_CONFLICT
```

Un terminale EDITOR in workspace DEV o TECHNICAL_DESIGNER può preparare specifiche, handoff e operazioni, e può usare capacità realmente read-only. Non può modificare asset tramite MCP.

Una sessione VALIDATION può usare Unreal/PIE per misurare, ma non ripara asset durante il sign-off. Un finding torna a EDITOR; dopo il fix si crea una nuova evidenza su input rimisurato.

La final validation di asset presenti soltanto nel workspace MAIN può essere eseguita da una successiva sessione `VALIDATION` nello stesso workspace MAIN, dopo che EDITOR ha salvato, chiuso Unreal, rilasciato il lease e prodotto l'handoff. Il workspace resta MAIN; cambia il ruolo della sessione. Quando serve dimostrare reload/persistenza, riaprire su input e SHA dichiarati.

---

## Lease Unreal just-in-time

Il lease è della risorsa, non del ruolo.

- Aprire un terminale o avviare Claude non acquisisce Unreal.
- `EDITOR` e `VALIDATION` possono lavorare senza lease finché non usano il motore.
- Il lease viene acquisito immediatamente prima di Editor, PIE, commandlet, Automation o MCP che richieda Unreal vivo.
- Il lease non è preemptive: nessuna sessione termina o forza quella attiva.
- Per questa revisione usa `BUSY` con informazioni owner e retry manuale. Non implementare una coda automatica se non esiste già una specifica testata.
- Il mode locale può essere informativo, mai autorizzativo.
- Il lease globale deve vivere sotto `%LOCALAPPDATA%\RefactorTactics\RT3\` ed essere condiviso fra le directory.
- Nessun `force unlock` deve cancellare ownership viva.
- Metadata stale possono essere recuperati soltanto quando owner e processi associati non sono vivi e non esiste un processo Unreal non attribuibile.
- Il rilascio deve verificare che i processi Unreal/PIE/commandlet avviati dalla sessione siano terminati. Se rimangono vivi, non dichiarare la risorsa libera.

I metadata del lease devono includere almeno:

```text
schema_version
lease_id
role
terminal_instance
workspace_id
workspace_root
project_path
task_id
operation
owner_pid
editor_pid, quando noto
mcp_endpoint o identificatore del bridge, senza secret
branch
head_sha
acquired_at_utc
heartbeat_utc, se implementato
```

Valuta criticamente se l'attuale file lock, mutex e lifecycle del processo proprietario garantiscono davvero queste proprietà. Non descrivere un heartbeat se non viene realmente implementato.

---

## Enforcement MCP obbligatorio

Prima di scrivere codice, determina come la Unreal/Epic MCP è configurata e invocata nel progetto corrente.

L'architettura desiderata ha due livelli:

1. **esposizione per workspace**:
   - nel workspace MAIN è disponibile il gateway MCP controllato;
   - negli altri workspace esporre soltanto capacità read-only;
   - se il server non supporta una modalità read-only affidabile, disabilitare completamente i tool Unreal mutanti fuori da MAIN;
2. **guard runtime**:
   - ogni chiamata mutante passa attraverso wrapper/proxy;
   - il wrapper ricontrolla ruolo, workspace registrato, task, branch, write-set, lease, project path, PID/endpoint e SHA immediatamente prima dell'invocazione;
   - dopo la chiamata verifica postcondizione, asset effettivamente modificati e stato del processo;
   - registra l'evento senza payload sensibili.

Se una configurazione per-workspace non può impedire il bypass del wrapper, dichiaralo come rischio aperto e scegli la mitigazione più forte realmente supportata. Non sostenere che uno script PowerShell “protegge la MCP” se Claude può ancora chiamare direttamente lo stesso tool mutante.

Il wrapper deve distinguere almeno:

- query read-only senza Unreal;
- query read-only che richiedono Editor vivo;
- avvio/controllo Editor o PIE;
- mutazione asset;
- validazione;
- tool sconosciuto.

Usa allowlist esplicite per le operazioni mutanti note. I tool nuovi/sconosciuti sono `DENIED_UNCLASSIFIED` finché classificati.

---

## Log task-specifico

Ogni acquisizione/rilascio lease e ogni chiamata MCP con side effect deve registrare in JSONL almeno:

```text
timestamp_utc
event
task_id
role
terminal_instance
workspace_id
workspace_root
branch
head_sha
lease_id
mcp_server
mcp_tool
operation_class
target_summary
result
modified_assets
error_code
```

Non registrare token, credenziali, URL firmati, header, prompt completi o payload asset sensibili. La scrittura del log deve essere concorrente-safe e non deve trasformare un fallimento dell'operazione in un falso successo. Documenta la retention o almeno il percorso e la responsabilità di pulizia.

Gli handoff persistiti devono riferirsi a task/issue, branch, SHA, asset modificati, evidenze e lease/event ID rilevanti.

---

## Modifiche attese al tooling

Riconcilia con i nomi e i file realmente presenti. Non creare duplicati se esiste già uno script equivalente.

1. Installer:
   - default/recommended `WorkspaceRole ALL`;
   - parametro esplicito `WorkspaceId MAIN|DEV|TECHNICAL_DESIGNER`;
   - registrazione sicura della root MAIN;
   - backup e scrittura atomica;
   - nessuna sovrascrittura cieca di configurazioni MCP o task locali.
2. `rt-terminal`:
   - imposta ruolo, istanza, root, workspace ID e task ID quando fornito;
   - avvia Claude senza acquisire Unreal;
   - carica esattamente un prompt di ruolo e un prompt wave compatibile quando richiesto.
3. `rtmode`/lease:
   - comandi espliciti `status`, `acquire` e `release`, oppure equivalenti chiaramente documentati;
   - `acquire` richiede operation e task ID per operazioni mutanti;
   - nessuna autorizzazione derivata dal mode locale.
4. Validation wrapper:
   - richiede ruolo VALIDATION e lease solo per le fasi che usano realmente Unreal;
   - preserva il mutex canonico già esistente;
   - non confonde `performed=0` con PASS.
5. MCP wrapper/proxy:
   - applica la capability matrix e la policy MAIN-only;
   - impedisce o segnala chiaramente ogni bypass residuo;
   - lega la chiamata all'istanza Unreal corretta.
6. VS Code tasks:
   - tutti i ruoli disponibili in ogni directory;
   - nessun `Start Claude` acquisisce automaticamente il lease;
   - task separati per stato/acquisizione/rilascio e, se utile, invocazione MCP sicura;
   - JSON valido e alias storici mantenuti solo se coerenti.
7. Prompt EDITOR:
   - distingue preparazione read-only da `MCP_ASSET_WRITE`;
   - richiede MAIN soltanto per l'authoring asset, non per l'esistenza del ruolo EDITOR;
   - vieta l'auto-acceptance del risultato scritto;
   - impone save/close/reopen quando la persistence è parte dell'oracolo.

---

## Compatibilità e migrazione

Mantieni compatibilità Windows PowerShell 5.1, quoting corretto dei path con spazi e nessuna dipendenza esterna non già approvata.

Aggiorna README/APPLY con una guida per tre directory simile a questa, usando i parametri definitivi realmente implementati:

```powershell
# Refactor Tactics Main
.\docs\rt-three-terminals\install-rt-terminals.ps1 `
  -RepoRoot (Get-Location).Path `
  -WorkspaceRole ALL `
  -WorkspaceId MAIN

# Refactor Tactic Dev
.\docs\rt-three-terminals\install-rt-terminals.ps1 `
  -RepoRoot (Get-Location).Path `
  -WorkspaceRole ALL `
  -WorkspaceId DEV

# Refactor Tactics Technical Designer
.\docs\rt-three-terminals\install-rt-terminals.ps1 `
  -RepoRoot (Get-Location).Path `
  -WorkspaceRole ALL `
  -WorkspaceId TECHNICAL_DESIGNER
```

Spiega come migrare da installazioni che avevano un ruolo fisso per directory e come tornare indietro usando i backup. Non eseguire automaticamente la migrazione su altre directory della macchina.

---

## Test obbligatori

Esegui prima test statici e simulazioni senza avviare Unreal.

Verifica almeno:

1. ogni workspace `ALL` apre DEV, EDITOR e VALIDATION;
2. `Start Claude EDITOR` e `Start Claude VALIDATION` non acquisiscono il lease;
3. EDITOR in workspace DEV non può effettuare `MCP_ASSET_WRITE`;
4. EDITOR in TECHNICAL_DESIGNER non può effettuare `MCP_ASSET_WRITE`;
5. EDITOR in MAIN senza lease viene bloccato;
6. EDITOR in MAIN sul branch `main` viene bloccato per asset write;
7. EDITOR in MAIN senza task ID o write-set viene bloccato;
8. EDITOR in MAIN con project path, PID o endpoint MCP discordanti viene bloccato;
9. EDITOR in MAIN con tutti i prerequisiti può raggiungere il gateway in modalità dry-run/mock, senza modificare veri asset;
10. VALIDATION non può effettuare asset write;
11. un secondo lease concorrente è `BUSY` e mostra owner/task/workspace senza secret;
12. metadata stale non equivalgono a lease vivo;
13. Unreal vivo senza owner verificabile blocca l'acquisizione;
14. il lease non viene dichiarato rilasciato mentre un processo Unreal posseduto è ancora vivo;
15. un tool MCP sconosciuto e potenzialmente mutante è negato;
16. il percorso MCP mutante diretto non è disponibile fuori MAIN oppure il rischio residuo è dimostrato e documentato;
17. log JSONL valido, concorrente-safe e privo di token/segreti;
18. `tasks.json` e configurazioni MCP hanno sintassi valida;
19. gli script fanno parsing in Windows PowerShell 5.1;
20. path con spazi funzionano;
21. installer ripetuto è idempotente e conserva backup/configurazioni non possedute;
22. link e riferimenti documentali non sono rotti;
23. `git diff --check` è pulito.

Se Windows PowerShell, Claude Code, la MCP reale o Unreal non sono disponibili, marca i relativi test `NOT RUN` con motivo. Non trasformare simulazioni Linux/PowerShell Core in PASS per Windows PowerShell 5.1. Non aprire Unreal o modificare asset reali per questa revisione tooling/documentale salvo autorizzazione esplicita.

---

## Git e sicurezza

- Non lavorare direttamente su `main`.
- Crea un branch focalizzato, per esempio `feat/rt3-mcp-main-asset-policy`.
- Non usare reset, checkout distruttivi, clean o sovrascritture di file utente.
- Non includere modifiche non correlate già presenti nel working tree.
- Non versionare segreti, path personali assoluti o configurazioni macchina generate.
- Usa Conventional Commits.
- Non fare merge e non chiudere issue senza mandato.
- Crea una PR soltanto se le credenziali e il mandato lo permettono; altrimenti prepara branch, commit e testo PR.

## Criteri di accettazione

La revisione è completa soltanto se:

- nessuna directory è vincolata a un ruolo operativo;
- MAIN è un'identità di workspace verificabile e non un confronto fragile sul nome;
- asset MCP write è possibile esclusivamente con `EDITOR + MAIN + task branch + task/write-set + lease + binding MCP`;
- avviare Claude non occupa Unreal;
- una sola sessione usa Unreal sulla macchina;
- la MCP non può collegarsi silenziosamente all'Editor sbagliato;
- i tool mutanti sconosciuti sono negati;
- VALIDATION resta indipendente;
- documentazione, prompt, task, installer e wrapper descrivono e applicano la stessa policy;
- test realmente eseguiti e `NOT RUN` sono distinti;
- esiste una procedura di migrazione e rollback utilizzabile.

## Output finale richiesto

Riporta:

1. critica iniziale e decisioni adottate;
2. branch e SHA iniziale/finale;
3. write-set effettivo;
4. file modificati e autorità di ciascuno;
5. capability matrix finale;
6. comportamento del lease;
7. enforcement MAIN-only e protezione dai bypass MCP;
8. test con comando ed esito reale;
9. test `NOT RUN`;
10. rischi residui e limiti non risolti;
11. procedura esatta per aggiornare le tre directory;
12. rollback;
13. commit SHA e URL PR, oppure motivo per cui non sono stati creati.
