# Prompt launcher per Claude Code

Copia questo prompt in Claude Code dalla root di `refactor-tactics-main`:

```text
Stiamo implementando la Visual Slice v0.1 di RefactorTactics.

Leggi prima AGENTS.md e CLAUDE.md. Poi apri il file task che ti indico sotto e trattalo come piano operativo, non come fonte normativa superiore al repository.

Prima di implementare:
- misura branch/HEAD/git status;
- leggi l'issue GitHub correlata e i commenti correnti;
- verifica il codice e i test reali;
- segnala eventuali differenze fra il task e lo stato corrente.

Non lavorare dalla memoria e non espandere lo scope.

TASK DA ESEGUIRE:
<INCOLLA QUI IL PERCORSO, es. docs/tasks/visual-slice/01_PRECHECK_GBX_1094.md>

Completa un solo task per volta.
Alla fine usa esattamente il formato handoff indicato nel task.
Non iniziare il task successivo finché non ti do conferma.
```
