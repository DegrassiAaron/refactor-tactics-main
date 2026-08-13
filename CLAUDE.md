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

Usa search/grep prima di aprire file lunghi. `docs/src/` è input/north-star non ancora consumato: non usarlo
come autorità implicita. `docs/archive/` è storico, e `docs/archive/src/` conserva i sorgenti già recepiti —
utile per la provenienza, mai per la regola.

## 2. Pin rapidi

- UE **5.8.1**; v0.1 **2v2 offline vs bot**; hex multilivello; roster **Gadget/Phase/Riktor/Wraith**
  (**D-120**). `Hero.Flux` / `Hero.Riva` / `Hero.Bastion` / `Hero.Vektor` restano gli **Stable ID**: non
  sono nomi e non si rinominano (blocker [#716](https://github.com/DegrassiAaron/refactor-tactics-main/issues/716)).
  Gate: `python scripts/check-docs-naming.py --check`.
- Fasi: `Planning → Prep → Dash → Blast → Move → Cleanup`; Move normale resta dopo Blast.
- Un solo substrato: `FRTCellId`; no gameplay quadrato parallelo.
- **No GAS nella v0.1**: `URTActionData` / `URTHeroData` / `URTEquipmentData`.
- Azioni generiche (sette, **D-025**): `Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`.
- Ownership contenuti (**D-029**): abilità → singolo owner; interazioni → sistemi; sinergie/fazioni/scenari →
  esempi. Niente ability di coppia né branch `if HeroA && HeroB`.
- **Sprint = profilo Move, non Dash**.
- Reazioni: `Opportunity → Commit`; Fast Reaction **3,0 s**, timeout **HOLD**.
- `Vektor.InterceptShot` = thin slice Predictive v0.1.
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
- Prima di cancellare/rinominare cerca riferimenti C++, config, reflection, soft reference e Blueprint.
- Un handoff/audit non è autorità e non autorizza da solo a implementare tutto ciò che contiene.

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
