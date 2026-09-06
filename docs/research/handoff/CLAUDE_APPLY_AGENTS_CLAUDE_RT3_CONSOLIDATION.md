> ⛔ **NOTA DEL REPOSITORY — NON RIESEGUIRE.** Aggiunta il 2026-09-06 spostando il file dalla radice;
> il testo sotto è quello ricevuto, invariato.
>
> **Questo mandato è già stato applicato**, su entrambi i lati:
>
> | Metà | Applicata da | Evidenza |
> |---|---|---|
> | `AGENTS.md` | `0a343a12`, PR [#2558](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2558) *(+110 righe)* | contiene le tre figure `DEV · EDITOR · VALIDATION` con responsabilità e limiti, e `DEV-LEAD`/`DEV-MAIN`/`DEV-TEST` dichiarati **funzioni DEV dentro una wave, non figure aggiuntive** |
> | `CLAUDE.md` | lo stesso `0a343a12` *(±239)*, poi `8897d94e` *(±956)* e `4fb86ea5` (PR [#2560](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2560)) | porta `RT_TERMINAL_*`, `RT_WORKSPACE_*`, `rtstatus`, il caricamento di `RT3_CONTRACT.md` e di un solo `TERMINAL_*.md`, `ROLE_MISSING`/`ROLE_CONFLICT` fail-closed e le sezioni `DEV`/`EDITOR`/`VALIDATION` |
>
> I due blob di partenza del Preflight **non sono più quelli correnti**: `AGENTS.md` era `8ee70da0` a
> `f80cf57f` (2026-09-05 18:26), oggi è `142a13c6`; `CLAUDE.md` era `f2a45527`, oggi è `b9946993`.
> Il passo 3 del Preflight, eseguito oggi, **fallisce per costruzione**.
>
> 📌 **Perché è qui e non in `docs/archive/src/`**: l'archivio è un registro, e ogni sua riga porta il link
> a un **referto** che dichiara cosa il kit ha prodotto e cosa aveva sbagliato. Per questo mandato nessun
> referto esiste — cercato in `docs/roadmap/plans/`, e nessun documento del repository lo cita. Archiviarlo
> senza referto lo farebbe figurare come revisionato quando non lo è.
>
> ⚠️ Il mandato **fratello** `CLAUDE_CONSOLIDATE_RT3_MCP_MAIN_ASSET_POLICY.md` è stato **cancellato** da
> `8897d94e` dopo l'applicazione, non archiviato. Questo è l'ultimo superstite della famiglia, ed è rimasto
> in radice perché entrato con `218716e5` — un commit intitolato *«docs»* che portava anche i tre `.bak`
> dell'installer, rimossi da PR [#2562](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2562).

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
