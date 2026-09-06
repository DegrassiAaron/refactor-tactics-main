# Claude Code — consolida AGENTS.md e CLAUDE.md sul modello RT3

Lavora dalla root di `refactor-tactics-main` in un terminale DEV.

## Obiettivo

Integrare le versioni candidate `AGENTS.md` e `CLAUDE.md` fornite con questo bundle, consolidando il modello operativo nelle tre figure:

```text
DEV · EDITOR · VALIDATION
```

La modifica deve ridurre duplicazioni e non creare una quarta source of truth.

## Preflight

1. Leggi integralmente gli `AGENTS.md` e `CLAUDE.md` correnti del repository.
2. Misura `git status`, branch, `HEAD` e `origin/main`.
3. Verifica se i blob di partenza coincidono con:
   - `AGENTS.md`: `8ee70da0551fb3b53f1931908463c7ae2c79c889`;
   - `CLAUDE.md`: `f2a45527e07065a14ec83ad959f0daea48203f73`.
4. Leggi:
   - `docs/rt-three-terminals/README.md`;
   - `docs/rt-three-terminals/prompts/RT3_CONTRACT.md`;
   - i tre `TERMINAL_*.md`;
   - i prompt `WAVE_DEV_LEAD.md`, `WAVE_EDITOR.md` e `WAVE_VALIDATION.md`.
5. Se i file correnti sono avanzati, non sostituirli alla cieca: integra semanticamente le differenze candidate.
6. Dichiara write-set limitato a `AGENTS.md` e `CLAUDE.md`, salvo un riferimento rotto direttamente causato dalla modifica.

## Autorità da ottenere

### AGENTS.md

È l'autorità tool-agnostic sulle tre figure e deve definire:

- responsabilità e limiti di DEV, EDITOR e VALIDATION;
- `DEV-LEAD`, `DEV-MAIN`, `DEV-TEST` come funzioni DEV di wave, non figure aggiuntive;
- una sola figura per sessione;
- ruolo non dedotto dal nome della directory;
- più terminali nella stessa directory = working tree condiviso, non isolamento;
- directory separate = working tree separati, ma risorse macchina condivise;
- coordinamento tramite branch, SHA e handoff persistiti, non copie locali;
- esclusione reciproca fra EDITOR e VALIDATION quando occupano Unreal;
- catena canonica `DEV-LEAD → EDITOR → VALIDATION`;
- Validation Window preliminare consentita ma non equivalente al sign-off finale;
- handoff minimo e condizioni fail-closed;
- puntatori al contratto RT3 senza duplicarne matrice e schema completo.

### CLAUDE.md

È solo overlay Claude-specifico e deve definire:

- lettura delle variabili `RT_TERMINAL_*` e `RT_WORKSPACE_*`;
- `rtstatus` all'avvio quando disponibile;
- caricamento di `RT3_CONTRACT.md` e di un solo `TERMINAL_*.md`;
- un solo prompt `WAVE_*.md` compatibile per sessione;
- `ROLE_MISSING` e `ROLE_CONFLICT` fail-closed;
- comportamento specifico di Claude per asset, MCP, test, lavoro parallelo e Git;
- routing rapido DEV/EDITOR/VALIDATION.

Non deve mantenere copie dei pin e guardrail già posseduti da `AGENTS.md`. Sostituisci le duplicazioni con riferimenti precisi, mantenendo però i numeri delle sezioni 2–10 per non rompere i riferimenti esistenti, incluso `RT3_CONTRACT.md` → `CLAUDE.md §6`.

## Invarianti

- Non modificare semantica gameplay, DoD, tassonomia issue o priorità.
- Non assegnare le directory `Main`, `Dev` o `Technical Designer` automaticamente a un ruolo.
- Non imporre esattamente tre terminali: le figure sono tre, le istanze DEV possono essere N.
- Non promuovere EDITOR a validatore di privacy/determinismo.
- Non permettere a VALIDATION di riparare e approvare autonomamente il proprio fix.
- Non copiare la matrice di `RT3_CONTRACT.md` dentro `AGENTS.md` o `CLAUDE.md`.
- Non aggiornare snapshot, conteggi o pin volatili.
- Documentazione in italiano; identificatori tecnici invariati.

## Verifiche

1. Controlla i link Markdown modificati.
2. Esegui il gate documentale applicabile, incluso `node tools/radar/doc-links.ts --check`.
3. Cerca riferimenti esistenti a `CLAUDE.md §2`…`§10` e verifica che restino semanticamente validi.
4. Verifica che `CLAUDE.md §6` continui a descrivere la validità dei test richiesta da `RT3_CONTRACT.md`.
5. Cerca duplicazioni residue di roster, loop, azioni, coordinate, privacy e guardrail fra i due file.
6. Cerca conflitti con `RT3_CONTRACT.md` sui ruoli e sulla sequenza degli handoff.
7. Registra come `NOT RUN` qualsiasi gate non eseguito; non serve avviare Unreal per due file Markdown.

## Git e output

- Crea un branch focalizzato, per esempio `docs/rt3-agents-claude-consolidation`.
- Commit Conventional Commits.
- Apri PR verso il parent branch verificato.
- Non chiudere issue senza owner confermato.

Riporta:

- branch e SHA;
- differenze integrate rispetto ai candidati;
- duplicazioni rimosse;
- conflitti risolti;
- gate eseguiti;
- `NOT RUN`;
- rischi residui;
- URL della PR.
