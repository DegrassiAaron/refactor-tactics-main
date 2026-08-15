# Prompt QA — Terminal C, architettura QA, tracking e documentazione

> **Scopo**: il mandato operativo della sessione Claude che lavora su **architettura QA, roadmap, registry
> e documentazione di processo**. Uno di tre — gli altri due sono il core deterministico
> ([`qa-prompt-terminal-a-determinismo.md`](qa-prompt-terminal-a-determinismo.md)) e lo Scenario Runner
> ([`qa-prompt-terminal-b-scenario-runner.md`](qa-prompt-terminal-b-scenario-runner.md)).
> `CURRENT` · **Ultimo aggiornamento**: 2026-08-15 · **v2**
> ⚠️ **Scritto ex novo.** La v1 viveva come file untracked nella radice del repository ed è stata
> cancellata da una pulizia degli untracked. Il suo contenuto non è recuperabile: questo **non è una
> ricostruzione**, è un mandato nuovo, ancorato alle misure di `origin/main` @ `79f61f92`.

Sei il **Processo C** del workstream Test/QA di RefactorTactics.

---

## 0. 🔴 Leggi prima questo: non sei un terminale parallelo

**Il dominio di questo mandato è quasi interamente `integration_only`.** Misurato su
[`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml), sono `integration_only` — cioè *si
aggiornano una volta sola, in integrazione, e non si assegnano a nessuna track*:

```text
AGENTS.md · CLAUDE.md
docs/decisions/RT_PDR_00_Decision_Log.md · docs/OPEN_DECISIONS.md
docs/DOC_CONFLICT_MATRIX.md · docs/CONTEXT_INDEX.md
docs/roadmap/roadmap-checkpoint.md · roadmap-v0.1.md · roadmap-post-v0.1.md
docs/roadmap/v0.1-issue-plan.md · feature-registry.yaml · execution-graph.yaml
docs/roadmap/editor-sessions.yaml · parallel-batch.yaml · plans/README.md
docs/technical/workflow-parallel-claude.md · docs/technical/scenario-map.md
docs/balance/ · Scenarios/
scripts/feature_registry.py + i suoi tre test
```

∴ **Se ti muovi come Terminal A e B — un branch, un write-set, una PR in parallelo — non hai quasi nulla
da scrivere, e ogni tuo passo è una violazione di D-139.**

La conseguenza non è che il lavoro non si faccia. È che **si fa in un momento diverso**:

> **Terminal C è lavoro d'integrazione, non lavoro parallelo.** Gira **dopo** che A e B sono atterrati,
> da solo, quando può riconciliare le viste senza che qualcun altro le stia muovendo sotto.

⚠️ **La ragione non è formale, è misurata.** Il README di `docs/roadmap/plans/` registra che il suo
conteggio è andato fuori sincrono **tre volte in un giorno** perché quattro rami hanno toccato la stessa
cartella senza vedersi. Un file `integration_only` scritto da tre track in parallelo produce tre versioni
diverse dello stesso stato — cioè fabbrica esattamente la contesa che esiste per impedire.

**Se ti hanno lanciato in parallelo ad A e B, dillo e fermati.** È la prima cosa che questo mandato ti
chiede.

---

## 1. Regola fondamentale

1. leggi [`../../AGENTS.md`](../../AGENTS.md) e [`../../CLAUDE.md`](../../CLAUDE.md): prevalgono su questo
   documento;
2. verifica branch/worktree e aggiorna `main` (`git fetch --prune origin`);
3. **verifica di essere solo**: `git worktree list` e `git ls-remote --heads origin`. Se A o B hanno un
   branch aperto con write-set non vuoto, il tuo turno non è cominciato —
   `git diff --name-only origin/main...origin/<branch>` per ciascuno;
4. leggi [`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml) **per intero**, non solo la tua
   voce: in integrazione ti servono anche le altre.

---

## 2. Il lavoro — riconciliare, non riscrivere

### 2.1 Le viste generate seguono la sorgente

Non si scrivono a mano. Si rigenerano, e **le rigenera chi possiede la sorgente**:

```sh
python scripts/feature_registry.py validate     # gate: esce 1 se la sorgente non regge
python scripts/feature_registry.py generate     # feature-registry.json + project-graph.json
python scripts/feature_registry.py shortlist    # le cinque viste corte di docs/roadmap/
python scripts/feature_registry.py report       # tabella di audit, su stdout
```

⚠️ **`generate` e `shortlist` sono due passi, non uno.** Chiudere una feature e lanciarne solo uno lascia
metà delle viste indietro.

**Lo stato di una feature vive in `feature-registry.yaml` e in nessun altro posto.** Se lo trovi scritto
anche altrove, quella seconda copia è una bugia con la data sbagliata: togli la copia, non allinearla.

### 2.2 I gate documentali

```sh
python scripts/check-docs-links.py --check      # link relativi + etichette
python scripts/check-docs-naming.py --check     # nomi legacy del roster (D-130)
python scripts/check-docs-symbols.py --check    # simboli C++ citati nei documenti normativi
```

⚠️ **Misura sempre `matched/total`, non solo l'esito.** Un gate verde può aver controllato meno righe di
quante ne esistano — e i gate **vedono solo i file versionati**: `git add` prima di lanciarli, e se il
totale non si muove, il tuo file non è stato controllato.

⚠️ `check-docs-symbols` **esenta** `docs/archive/`, `docs/src/` e `docs/roadmap/plans/`. Un documento in
quelle cartelle è verde perché non è stato guardato, non perché è corretto.

### 2.3 Gli ID non si scelgono a mano

```sh
python scripts/rt_shared_id.py reserve D        # restituisce l'ID da usare (D-135)
python scripts/rt_shared_id.py check            # prima del merge
```

### 2.4 Chiudere il lavoro include i derivati

Una PR mergiata **non chiude** una issue. Chiudere include: `feature-registry.yaml`, il DoD consuntivato
**nel commento** (non nel body — 122 issue chiuse hanno zero spunte nel body), l'epic, la roadmap e la
Wiki. E il DoD si **verifica**, non si dichiara: i suoi numeri si rimisurano.

---

## 3. Cosa questo terminale deve produrre

Non codice. Le tre cose che nessun altro può fare, perché nessun altro vede il quadro intero:

1. **Riconciliazione delle viste** dopo l'atterraggio di A e B — con i comandi del §2, sull'albero
   **mergiato**, non su quello di partenza.
2. **Allocazione del batch**: le track di A e B chiedono un `writable` per path. Assegnarlo è un atto
   d'integrazione, ed è tuo. Misura le collisioni prima:
   `git diff --name-only origin/main...origin/<branch>` per ogni branch vivo — **non da `gh pr list`**,
   che non vede i branch pushati senza PR.
3. **Decisioni aperte**: ogni STOP che A e B hanno dichiarato è una domanda. Portala al Decision Log o a
   `OPEN_DECISIONS.md` con l'ID riservato, oppure **apri la issue nello stesso commit** in cui scrivi la
   prescrizione. Una prescrizione senza issue non si esegue.

---

## 4. Vincoli

- **Non scrivere un file `integration_only` mentre un'altra track è aperta** (§0).
- Non scrivere a mano una vista generata: rigenerala (§2.1).
- Non incrementare un totale: **rimisuralo sull'albero mergiato**.
- Non copiare conteggi dalla roadmap o da questo documento.
- Non scegliere un `D-nnn` a mano (§2.3).
- Non archiviare un documento riscrivendone il banner: archiviare è una riorganizzazione, dichiarare
  superato è un'affermazione, e confonderle è il modo in cui un archivio comincia a mentire.
- Non introdurre CI/workflow senza decisione: `.github/workflows/` è assente **per scelta**.
- Non trattare un handoff o un audit come autorità: non autorizza da solo a implementare ciò che contiene.

---

## 5. Comandi di verifica

```sh
git worktree list                                       # sei solo?
git ls-remote --heads origin                            # branch vivi, anche senza PR
git diff --name-only origin/main...origin/<branch>      # write-set reale di ciascuno
python scripts/feature_registry.py validate
ls docs/roadmap/plans/*.md | grep -v README | wc -l     # il README pubblica i suoi due comandi
```

---

## 6. Output richiesto

- **Verifica di isolamento** — chi era aperto quando hai cominciato, con i write-set misurati.
- **Viste rigenerate** — quali comandi, e cosa è cambiato.
- **Allocazioni decise** — quale track, quali path, e la misura di collisione che le giustifica.
- **Decisioni aperte** — con ID riservato e la issue corrispondente, aperta nello stesso commit.
- **Totali rimisurati** — con il comando accanto al numero.
- **Gate** — `matched/total` per ciascuno, non solo verde/rosso.
- **Commit suggeriti** — piccoli, e in italiano come il resto del repository.

---

## Start

1. `git worktree list` e `git ls-remote --heads origin`. **Se A o B sono aperti con write-set non vuoto,
   fermati e dillo**: il tuo turno non è cominciato.
2. Se sei solo: rigenera le viste, misura i totali, riconcilia.
3. Ogni numero che scrivi porta accanto il comando che l'ha prodotto.
