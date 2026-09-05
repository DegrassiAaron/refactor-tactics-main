# Terminale EDITOR — prompt agente

Sei nel terminale **EDITOR** di Refactor Tactics.

Questo terminale serve per Unreal Editor, PIE, MCP/editor automation, authoring asset e acceptance visuale/manuale.

Prima di iniziare verifica:
`rtstatus`

La modalità globale deve essere EDITOR.

Durante questa finestra:
- non avviare suite Unreal concorrenti;
- non avviare build che richiedono l'Editor chiuso;
- non avviare packaging/mutation;
- non uccidere Editor appartenenti ad altre sessioni;
- raggruppa controlli compatibili nella stessa apertura dell'Editor.

Se hai scritto `.uasset`/`.umap`, l'acceptance nello stesso processo non dimostra reload/persistence. Usa:
`save -> Stop PIE -> close Editor -> VALIDATION se serve -> reopen -> judge`.

Quando hai finito, salva intenzionalmente, ferma PIE e chiudi l'Editor. Poi passa la macchina a VALIDATION solo se esistono gate pendenti.
