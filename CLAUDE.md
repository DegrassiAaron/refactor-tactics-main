# CLAUDE.md — RefactorTactics

Overlay operativo per **Claude Code / SuperClaude**.
Le regole condivise del repository sono in **[`AGENTS.md`](AGENTS.md)**: leggilo prima di lavorare.
Questo file resta volutamente corto per ridurre duplicazioni e drift.

## 1. Context protocol

Non lavorare dalla memoria del progetto. Verifica branch/HEAD, task, codice, test e documenti owner.

Carica il contesto in questo ordine, **solo quando pertinente**:

1. `AGENTS.md`.
2. Decisioni/invarianti: `docs/product/piano-canonico-mvp.md`.
3. Decisioni recenti: `docs/decisions/RT_PDR_00_Decision_Log.md` + ADR applicabili.
4. Supersessioni/conflitti: `docs/DOC_CONFLICT_MATRIX.md` + `docs/OPEN_DECISIONS.md`.
5. Stato/scope: `docs/roadmap/roadmap-checkpoint.md` + `docs/roadmap/roadmap-v0.1.md`.
6. Feature: specifica owner + cataloghi `docs/balance/` + test + implementazione esistente.

Usa search/grep prima di aprire file lunghi. `docs/research/` è input/north-star non ancora consumato: non
usarlo come autorità implicita. `docs/archive/` è storico, e `docs/archive/src/` conserva i sorgenti già
recepiti — utile per la provenienza, mai per la regola.

> ⚠️ **Era `docs/src/` fino al 2026-08-19, e quella cartella non esiste più** (`git ls-files docs/src` → zero,
> [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165)). Chi seguiva questa riga
> cercava una cartella vuota e non leggeva mai la casella di posta reale.

Un `.pdf` non è **mai** autoritativo (**D-009**): resta fuori dal preflight e si apre solo per provenienza,
rationale storico o confronto richiesto; se è l'export di una spec Markdown corrente, vince il Markdown. La
sigla `PDR` nel nome di un file Markdown non lo rende storico — `docs/decisions/RT_PDR_00_Decision_Log.md` è
canonico perché è l'owner corrente. Regola estesa in [`AGENTS.md`](AGENTS.md).

## 2. Pin rapidi

- UE **5.8.1**; v0.1 **2v2 offline vs bot**; hex multilivello; roster **Gadget/Phase/Riktor/Wraith**
  (**D-120**). I nomi legacy sono **usciti dal repository** (**D-130**): gli `Hero.<Nome>` sono stati
  rinominati e i venti token abilità sono atterrati su `Hero.<Nome>.<Abilità>` **senza redirect** — **D-134**
  ha cancellato `ResolveLegacyActionId`, quindi non esiste una doppia verità da risolvere in lettura.
  ✅ Le cinque fette del piano sono chiuse (#753–#757) e il gate è verde **senza esenzioni**: oggi un nome
  legacy che ricompare è un difetto, non un residuo da tollerare —
  [`docs/technical/piano-migrazione-roster.md`](docs/technical/piano-migrazione-roster.md).
  ⛔ Il gate che lo controllava — `scripts/check-docs-naming.py` — è uscito con **D-182** (2026-08-21):
  oggi un nome legacy che ricompare non lo segnala nessuno.
- Fasi: `Planning → Prep → Dash → Blast → Move → Cleanup`; Move normale resta dopo Blast.
- Un solo substrato: `FRTCellId`; no gameplay quadrato parallelo.
- **No GAS nella v0.1**: `URTActionData` / `URTHeroData` / `URTEquipmentData`.
- Azioni generiche (sette, **D-025**): `Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`.
- Ownership contenuti (**D-029**): abilità → singolo owner; interazioni → sistemi; sinergie/fazioni/scenari →
  esempi. Niente ability di coppia né branch `if HeroA && HeroB`.
- **Sprint = profilo Move, non Dash**.
- Reazioni: `Opportunity → Commit`; Fast Reaction **3,0 s**, timeout **HOLD**.
- `Hero.Wraith.InterceptShot` = thin slice Predictive v0.1.
- High Ground: nessun bonus numerico alla vista in v0.1.
- Formato competitivo finale non deciso: 3v3 è baseline, 4v4 stress test.

Il dettaglio resta negli owner documentali; non duplicarlo qui.

## 3. Classifica il task

**Documentale/analitico** (`/sc:brainstorm`, `research`, `design`, `workflow`, `analyze`, `estimate`, panel,
`troubleshoot` senza `--fix`): produci l'output richiesto e **non passare automaticamente al codice**.

**Esecutivo** (`/sc:implement`, `task`, `improve`, `cleanup`, `test`, `build`, `git`, `troubleshoot --fix`):
verifica prima codice/test esistenti, poi applica il **diff minimo**. Niente refactor opportunistici.

Per implementazioni non banali, preflight breve:

**Obiettivo · Stato verificato · Assunzioni · File · Approccio · Rischi · Test**

## 4. Guardrail Claude

- Non inventare API Unreal: verifica la **5.8.1** e le firme realmente presenti.
- Simulazione/authority in C++; presentazione/configurazione in Blueprint/Data dove appropriato.
- Niente `Delay`, montage, Tick o `DeltaTime` per decidere sequencing competitivo.
- Niente dipendenza dall'ordine di `TMap`/`TSet`.
- Niente branch per eroe nel core quando il comportamento può essere data-driven/componibile.
- Nei test non aggirare il gameplay con `SetActorLocation`, `ApplyDamage` o `if (IsTest)` che salta la regola.
- Non modificare `.uasset`/`.umap` a mano; i passi Editor restano verifiche manuali finché non eseguiti.
  I binari sono **human-first**: li tocchi solo su richiesta esplicita e attraverso Unreal. Due `.uasset`
  non si fondono, quindi un binario si modifica da un lavoro solo per volta.
- Prima di cancellare/rinominare cerca riferimenti C++, config, reflection, soft reference e Blueprint.
- Un handoff/audit non è autorità e non autorizza da solo a implementare tutto ciò che contiene.
- **Sviluppo sequenziale** (**D-178**): una sessione esecutiva, una working directory, un branch alla
  volta. Niente worktree per parallelizzare: un task troppo grosso si spezza in issue che si fanno in
  fila. Se due task condividono una working directory, dillo invece di conviverci.
- `D-nnn` si legge dall'ultimo assegnato nel Decision Log e si **riverifica prima del merge**: una PR
  aperta che rivendica lo stesso ID con una tesi diversa è una collisione, e rinumeri la seconda.
- Prima del merge: `git fetch --prune origin`, poi `gh pr list --state open` per gli ID in volo.

## 5. Decision Boundary

Una finestra live non è un'attesa del resolver:

`Resolve segment → Opportunity → Decision Boundary → response → Validate/Commit → next segment`

Visual può rallentare la presentazione; Fast/Headless risponde subito tramite policy. Il risultato logico non
dipende dal tempo reale. Non inviare al client trigger futuri, percorsi futuri o intenti privati avversari.

## 6. Test e consegna

Ordine preferito: **test mirati → regressione correlata → suite richiesta dal DoD → build → PIE/packaged se gate**.
Per scenari integrati usa il **RT Scenario Test Harness** e il percorso reale
`Intent → Planning → Snapshot → Resolver → TurnLog`.

Non copiare conteggi test dalla roadmap: **misurali sul branch corrente** quando servono.
Prima di consegnare controlla `git status` e diff. Niente commit/push/merge/force/delete remoto senza richiesta
esplicita; niente file generati o segreti.

Output finale:

**Risultato · File modificati · Decisioni · Test/Build · Verifiche manuali · Limiti · Prossimo passo**

Non dichiarare “funziona”, “completo”, “sicuro”, “production ready” o “deterministico” senza evidenza.

## Lingua

Rispondi e commenta in **italiano**; identificatori e termini tecnici restano in English quando naturale.
Tutoring C++/UE solo se richiesto.
