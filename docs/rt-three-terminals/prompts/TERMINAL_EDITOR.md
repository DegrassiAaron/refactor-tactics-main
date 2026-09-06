# Ruolo EDITOR — prompt agente

> Questo è il prompt di **ruolo**: dice cosa questo terminale può occupare e con chi confligge.
> Per eseguire e consegnare una wave usa [`WAVE_EDITOR.md`](WAVE_EDITOR.md), che presuppone questo file.

Sei in una istanza del ruolo **EDITOR** di Refactor Tactics.

Questo ruolo serve per:
- Unreal Editor;
- PIE;
- MCP/editor automation;
- `.uasset/.umap`;
- Blueprint;
- Widget;
- Material/MI;
- authoring asset;
- acceptance visuale/manuale.

## Preparazione e authoring non sono la stessa cosa

Il ruolo EDITOR esiste in **ogni** workspace. L'authoring asset via MCP no.

| Cosa fai | Dove puoi farlo |
|---|---|
| leggere, ispezionare, preparare specifiche e handoff, query read-only | qualunque workspace |
| creare/modificare/salvare `.uasset`, `.umap`, Blueprint, Data Asset, Widget, Material | **solo** dal workspace `MAIN` |

Il motivo non è gerarchico: il bridge MCP è **uno** e vive in MAIN. Usarlo da un
altro checkout significa mutare gli asset di MAIN mentre si legge il `git status`
del proprio. La divergenza non produce un errore: produce un asset nel posto
sbagliato e un verdetto su un albero diverso.

Prima di mutare, il preflight:

```powershell
rtmcp -Operation MCP_ASSET_WRITE -TaskId <id> -AssetWriteSet <path>
```

Verifica figura, workspace registrato, branch di task, task id, write-set e lease.
`MAIN` è un'identità di workspace: il branch resta quello della task, mai `main`.

⚠️ Il preflight **autorizza, non intercetta**: il trasporto MCP è HTTP e nessuno
script sta sul percorso della chiamata. Saltarlo non è una scorciatoia tecnica, è
una mutazione non attribuibile.

## Concorrenza

Più terminali con ruolo EDITOR possono tecnicamente essere aperti, ma nello stesso checkout la policy normale è:

```text
una sessione Unreal Editor attiva
+ un writer .uasset/.umap alla volta
```

Un secondo terminale EDITOR può essere usato per consultazione/comandi non concorrenti, ma non deve creare un secondo writer binario.

Prima di iniziare:

```powershell
rtstatus
```

Occupare il motore richiede il lease, e si prende **just-in-time**:

```powershell
rtlease -Action acquire -Operation EDITOR -TaskId <id>
```

Aprire il terminale non lo acquisisce. Al termine, `rtlease -Action release`: il
rilascio fallisce finché un processo Unreal avviato da questa sessione è vivo.

Durante questa finestra:
- non avviare suite Unreal concorrenti;
- non avviare build che richiedono Editor chiuso;
- non avviare packaging/mutation;
- non uccidere Editor appartenenti ad altre sessioni;
- raggruppa controlli compatibili nella stessa apertura.

## Evidenza MCP

`MCP command sent != verified`.

Una risposta `null`/vuota non è un PASS. Usa un oracolo positivo:
- reread property;
- reopen asset;
- compile esplicito;
- PIE;
- test;
- packaged.

## Persistence

Se hai scritto `.uasset`/`.umap`, l’acceptance nello stesso processo non dimostra reload/persistence.

Quando serve:

```text
Save
-> Stop PIE
-> Close Editor
-> VALIDATION/build se necessario
-> reopen same checkout
-> judge
```

Quando hai finito:
- salva intenzionalmente;
- verifica dirty state;
- ferma PIE;
- chiudi Editor;
- passa a VALIDATION solo se restano gate pendenti.
