# Prompt Claude — integra RT Three Terminal Roles

Lavora nel repository `DegrassiAaron/refactor-tactics-main`.

## Obiettivo

Integra un workflow locale VS Code basato su **tre ruoli operativi con N terminali**:

1. **DEV — verde** — 1..N istanze;
2. **VALIDATION — giallo** — 1..N finestre possibili, ma un solo job Unreal attivo alla volta;
3. **EDITOR — blu** — normalmente una sola sessione Unreal attiva e un solo writer binario.

Il nome storico `rt-three-terminals` resta per compatibilità, ma “three” descrive i ruoli, non il numero massimo di finestre.

Esempio valido:

```text
DEV-1
DEV-2
DEV-3
DEV-4
VALIDATION-1
EDITOR-1
```

Tutte le istanze dello stesso slice usano:
- stesso repository root;
- stesso HEAD;
- stesso branch;
- stesso working tree;
- stessa modalità globale engine.

## Prima di modificare

Leggi:
- `AGENTS.md`;
- `scripts/rt-suite.ps1`;
- `docs/roadmap/editor-sessions.yaml`;
- issue/decisioni su mutex e `-WaitMinutes`;
- `docs/rt-three-terminals/README.md`;
- tutti i file del bundle corrente.

Verifica specificamente che ogni file referenziato dall’installer esista davvero.

## Vincoli

- niente CI/daemon/scheduler/servizi nuovi;
- non cambiare la semantica di validità di `rt-suite.ps1`;
- non uccidere Unreal Editor di altre sessioni;
- aprire un terminale NON cambia la modalità globale;
- default globale: DEV;
- in DEV niente waiting queue ordinaria per Unreal;
- NOT RUN non è PASS;
- full suite una volta per batch;
- niente catene lunghe non validate per modifiche ad alto rischio;
- più DEV nello stesso checkout non significa isolamento Git;
- un solo writer `.uasset/.umap` alla volta;
- più VALIDATION non autorizzano test Unreal concorrenti.

## Bundle VS Code

Il repository ignora `.vscode/`. Perciò i template versionati del bundle devono vivere sotto:

```text
docs/rt-three-terminals/payload/vscode/
```

e l’installer deve copiarli in:

```text
<RepoRoot>/.vscode/
```

Non usare `payload/.vscode/` come sorgente versionata.

L’installer deve verificare TUTTO il payload prima di iniziare a copiare, per evitare installazioni parziali.

## Comportamento atteso

### DEV

Consentito:
- code;
- test authoring;
- review;
- Git/GitHub;
- Node/Python/static tooling.

Possono esistere N istanze DEV.

Regole shared-working-tree:
- ownership file esplicita;
- staging per path;
- evitare `git add -A`, `commit -am`, reset/restore/clean/switch/rebase mentre altri DEV hanno modifiche.

### VALIDATION

Ordine:
1. static;
2. build;
3. targeted;
4. scenario;
5. full suite x1/batch.

`rtsuite` deve richiedere:
- role VALIDATION;
- engine mode VALIDATION.

La mutua esclusione reale resta nel percorso/mutex canonico.

### EDITOR

Consentito:
- UE Editor;
- PIE;
- MCP;
- authoring asset;
- visual acceptance.

Normalmente:
- una sessione Editor attiva;
- un writer binario.

Per asset scritti:
`save -> close -> validation/build se necessario -> reopen -> judge`.

## Identità istanza

Ogni `rt-terminal.ps1` deve avere un ID locale di istanza, preferibilmente derivato dal PID se non fornito:

```text
[DEV:18452] [ENGINE:DEV]
```

L’ID è solo visuale.

## Task VS Code richiesti

- `RT: Open DEV terminal`
- `RT: Open VALIDATION terminal`
- `RT: Open EDITOR terminal`
- `RT: Open role set (DEV + VALIDATION + EDITOR)`
- alias compatibile `RT: Open 3 terminals`

Il task DEV deve poter essere rieseguito in parallelo (`panel: new`, `instanceLimit > 1`).

## Verifiche minime senza Unreal

- parsing JSON dei template VS Code;
- installer preflight payload;
- install in directory test;
- due o più istanze DEV contemporanee;
- ID istanza differenti;
- una istanza VALIDATION;
- una istanza EDITOR;
- `rtstatus`;
- DEV -> VALIDATION -> EDITOR -> DEV;
- `rtsuite` bloccato da DEV;
- `rtsuite` bloccato da EDITOR;
- nessun processo Unreal avviato;
- nessun file imprevisto modificato.

## Output finale

Riporta:
- file modificati;
- verifiche eseguite;
- NOT RUN;
- rischi residui;
- conferma esplicita che il modello è “3 roles, N terminals”.
