# Prompt Claude — installa e integra i 3 terminali di Refactor Tactics

Lavora nel repository `DegrassiAaron/refactor-tactics-main`.

## Obiettivo

Integra un workflow locale VS Code con tre terminali distinti e visivamente riconoscibili:

1. **DEV — verde**
2. **VALIDATION — giallo**
3. **EDITOR — blu**

Il fine NON è aggiungere CI o altra infrastruttura. Il fine è ridurre il tempo in cui processi automatici occupano Unreal e impedire che suite/build si accodino durante il normale sviluppo o mentre l'utente vuole usare l'Editor.

## Prima di modificare

Leggi e rispetta:
- `AGENTS.md`, soprattutto le regole su Unreal/Editor/build/test;
- `scripts/rt-suite.ps1`;
- `docs/roadmap/editor-sessions.yaml`;
- le decisioni/issue che spiegano il mutex e `-WaitMinutes`, se disponibili.

Non inventare requisiti mancanti.

## Vincoli fondamentali

- Non introdurre GitHub Actions, Jenkins, daemon, scheduler, package manager o nuovi servizi.
- Non cambiare la semantica di validità di `rt-suite.ps1`.
- Non uccidere Unreal Editor aperti da altri processi/sessioni.
- Non far cambiare modalità globale semplicemente aprendo un terminale.
- Ruolo del terminale e modalità globale della macchina sono due concetti distinti.
- Default globale: `DEV`.
- In DEV gli agenti NON devono attendere che Unreal si liberi: niente `rt-suite -WaitMinutes ...` come comportamento ordinario.
- Se una validazione non può essere eseguita, registrarla come `NOT RUN / VALIDATION PENDING`; non fingere DONE.
- La full suite va eseguita una volta per batch integrato, non automaticamente dopo ogni issue.
- Modifiche ad alto rischio (resolver, replay format, TurnLog, serialization, map hash, determinism) non devono accumulare dipendenze non validate.

## File forniti nel bundle

Valuta e installa/adatta:
- `.vscode/settings.json`
- `.vscode/tasks.json`
- `scripts/rt-terminal.ps1`
- `scripts/rt-mode.ps1`
- `scripts/rt-suite-safe.ps1`

Il repository attuale ignora `.vscode/`: mantieni la configurazione VS Code locale salvo decisione esplicita contraria.

## Comportamento atteso

### DEV — verde

Consentito:
- edit codice;
- test authoring;
- git;
- review;
- Node/Python/static tooling;
- analisi headless che non usa Unreal.

Vietato come comportamento autonomo:
- UnrealEditor;
- UnrealEditor-Cmd;
- suite Unreal;
- packaging;
- mutation;
- build che richiedono l'Editor chiuso / monopolizzano Unreal.

### VALIDATION — giallo

Usare solo quando l'utente ha ceduto Unreal alla finestra di validazione.

Ordine preferito:
1. static/tool checks;
2. build;
3. targeted Unreal tests;
4. targeted Scenario Harness;
5. full suite una volta sul batch integrato.

`-WaitMinutes` può avere senso SOLO qui.

### EDITOR — blu

Consentito:
- Unreal Editor;
- PIE;
- MCP/editor automation;
- authoring asset;
- acceptance visuale/manuale.

Non consentire che suite/build concorrenti rubino Unreal.

Per asset scritti nella stessa sessione, rispettare:
`save -> close Editor -> build/suite se Source è cambiato -> reopen -> judge`.

## Implementazione richiesta

1. Verifica che i file forniti siano compatibili con il repository attuale.
2. Mantieni i tre terminali nominati e colorati in VS Code.
3. Mantieni lo stato globale locale in `.vscode/rt-engine-mode.txt`.
4. Fornisci i comandi:
   - `rtstatus`
   - `rtmode DEV`
   - `rtmode VALIDATION`
   - `rtmode EDITOR`
   - `rtsuite ...` protetto
5. `rtsuite` deve rifiutare l'esecuzione se:
   - non proviene dal terminale VALIDATION;
   - la modalità globale non è VALIDATION.
6. Non modificare direttamente `rt-suite.ps1` nella prima iterazione se un wrapper sottile basta.
7. Se proponi successivamente di integrare il controllo dentro `rt-suite.ps1`, separalo in una issue/step distinto e spiega impatti su self-test, exit code 2, mutex e `-WaitMinutes`.
8. Non hardcodare il percorso di Unreal Engine se il repository possiede già una fonte canonica per individuarlo.

## Verifiche minime

Senza aprire Unreal:
- parsing JSON di `.vscode/settings.json` e `.vscode/tasks.json`;
- avvio dei tre terminali;
- colori/nome corretti;
- `rtstatus`;
- cambio DEV -> VALIDATION -> EDITOR -> DEV;
- `rtsuite` bloccato da DEV;
- `rtsuite` bloccato dal terminale EDITOR;
- nessuna modifica involontaria del working tree oltre ai file previsti;
- nessun processo Unreal avviato durante questi test.

Se devi eseguire una vera suite Unreal, fermati e classificala `VALIDATION PENDING` finché non è stata aperta esplicitamente una Validation Window.

## Output finale

Riporta:
- file modificati;
- test eseguiti;
- test NON RUN;
- eventuali rischi residui;
- eventuale proposta di issue successiva per integrare il guard direttamente nei launcher canonici.
