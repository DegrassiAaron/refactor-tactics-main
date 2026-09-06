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

La modalità globale deve essere EDITOR.

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
