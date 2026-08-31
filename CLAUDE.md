# CLAUDE.md — RefactorTactics

Overlay operativo per **Claude Code / SuperClaude**.

> **Prima regola:** leggere [`AGENTS.md`](AGENTS.md).
>
> `CLAUDE.md` non duplica il contratto condiviso. Contiene solo il protocollo Claude-specifico e i pin ad alto rischio.

## 1. Context protocol

Non lavorare dalla memoria del progetto.

Prima di modificare:

1. misura `git status`;
2. identifica branch;
3. misura `HEAD`;
4. misura `origin/main`;
5. leggi `AGENTS.md`;
6. individua issue/task;
7. individua l'owner documentale;
8. cerca implementazione e test esistenti;
9. dichiara write-set;
10. dichiara rischi e verifiche previste.

Quando pertinenti controlla:

- `docs/product/piano-canonico-mvp.md`
- `docs/decisions/RT_PDR_00_Decision_Log.md`
- ADR applicabili
- `docs/DOC_CONFLICT_MATRIX.md`
- `docs/OPEN_DECISIONS.md`
- roadmap/checkpoint
- spec owner
- codice
- test

Usa search/grep per restringere il contesto prima di aprire documenti lunghi.

### Non usare come autorità implicita

- `docs/research/`
- `docs/archive/`
- PDF
- export
- handoff
- audit
- vecchi snapshot
- numeri copiati

Se due regole sono in conflitto, non riconciliarle a intuito.

Trova l'owner corrente.

## 2. Pin correnti

- Unreal Engine **5.8.1**.
- v0.1 = **2v2 offline vs bot**.
- Standard = **3v3** — D-256.
- 3v3 non cambia lo scope v0.1.
- 4v4+ = stress/scala.
- Roster = **Gadget · Phase · Riktor · Wraith**.
- Niente compatibilità implicita con nomi legacy rimossi.
- Nessun redirect legacy reintrodotto senza decisione.
- Mappa = **hex multilivello**.
- Coordinate = `FRTCellId`.
- Niente seconda griglia.
- Loop = `Planning → Prep → Dash → Blast → Move → Cleanup`.
- Azioni = `Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`.
- Traversal percorre celle.
- Transfer non percorre celle intermedie.
- Sprint = profilo Move.
- Sprint ≠ Dash.
- Reazioni = `Opportunity → Commit`.
- Fast Reaction baseline = **3,0 s**.
- Timeout = **HOLD**.
- Thin slice Predictive v0.1 = `Hero.Wraith.InterceptShot`.
- **No GAS nella v0.1**.
- High Ground non applica bonus numerici globali automatici a vista/danno.

Il dettaglio vive negli owner gameplay.

## 3. Guardrail architetturali

### Simulazione

La simulazione decide.

UI, VFX e animazioni mostrano.

Non usare per decidere l'esito:

- `DeltaTime`;
- timer real-time;
- timeline;
- animation callback;
- packet arrival order;
- ordine non deterministico di container.

### Spatial

Gameplay position:

`FRTCellId`

Presentation:

`FVector` / world transform.

Non:

- creare Actor per ogni esagono;
- duplicare world↔hex;
- duplicare pathfinding;
- duplicare LOS;
- duplicare targeting;
- creare un secondo modello spaziale.

### Gameplay ownership

C++ definisce cosa è possibile.

Data Asset/Blueprint configurano varianti e presentation.

Un'abilità ha un solo owner.

Non introdurre:

```text
PairBonus
ComboAbility
if (HeroA && HeroB)
```

quando l'interazione può emergere da uno stato o sistema condiviso.

## 4. Privacy

Client propone.

Authority valida e applica.

Non inviare il planning avversario ai client per poi nasconderlo graficamente.

UI e warning possono usare:

- stato pubblico;
- Team Knowledge;
- intenti della propria squadra.

Non possono usare:

- intenti privati avversari;
- trigger futuri;
- informazioni che il client non dovrebbe conoscere.

## 5. Unreal asset safety

Asset proprietari:

`/Game/RT/`

Non:

- editare `.uasset` a mano;
- editare `.umap` a mano;
- spostare asset Unreal da filesystem;
- modificare binari posseduti da un'altra sessione;
- sovrascrivere un working tree sporco;
- editare viste generate.

Usare:

- Content Browser;
- Fix Up Redirectors;
- Binary Asset Lease;
- write-set esplicito.

Il repository non usa Git LFS.

Owner:

[`docs/technical/tooling/convenzioni-contenuti-ue.md`](docs/technical/tooling/convenzioni-contenuti-ue.md)

## 6. Test

Entry point:

```powershell
./scripts/rt-suite.ps1
```

Per build, filtri, attesa e tool Node usa [`AGENTS.md`](AGENTS.md).

Non duplicare qui la lista operativa.

### Regole

Se il motore è occupato:

usa il comportamento dello script.

Non inventare watcher paralleli.

Una suite che vede cambiare:

- `HEAD`;
- working tree;
- binario;
- processi Unreal;

durante la misura è:

**NON VALIDA**.

Dopo un'attesa lunga:

**ricompila prima di registrare il verde.**

Se PIE/Editor/packaged non sono stati eseguiti:

**NOT RUN**.

Prima del merge:

verifica che il gate sia stato eseguito sul commit che stai realmente mergiando.

## 7. Lavoro parallelo

Il repository viene lavorato in parallelo.

Non assumere che rimangano stabili:

- branch;
- `HEAD`;
- `origin/main`;
- issue;
- PR;
- binari;
- shared ID.

Prima di creare qualcosa:

**SEARCH → REUSE / UPDATE → CREATE solo per gap reale.**

Non assegnare dalla memoria:

- `D-nnn`;
- Epic `Enn`;
- altri contatori condivisi.

Fetch e riverifica prima del merge.

## 8. Git

Branch focalizzati:

```text
feat/
fix/
refactor/
docs/
test/
```

Usare Conventional Commits.

Non fare operazioni remote distruttive senza autorizzazione esplicita.

Non confondere:

```text
file modificato
```

con:

```text
build/test/PIE/packaged verificato
```

## 9. Output dopo ogni pass

Riporta:

### Risultato

Cosa è cambiato realmente.

### File

File creati/modificati.

### Decisioni

Owner, ADR o Decision Log coinvolti.

### Verifiche

Build, test e tool effettivamente eseguiti.

### NOT RUN

Verifiche non eseguite.

### Rischi / aperti

Conflitti, limiti, decisioni o follow-up.

### Prossimo passo

Una sola azione consigliata.

Non dichiarare:

- funziona;
- completo;
- production ready;
- sicuro;
- deterministico;

senza evidenza.

## 10. Mappa rapida

| Percorso | Contenuto |
|---|---|
| `Source/RefactorTactics/` | Runtime C++ + Automation Tests |
| `Source/RefactorTacticsEditor/` | Tooling Editor |
| `Plugins/RTDeveloperTools/` | Developer tooling |
| `Content/RT/` | Asset proprietari |
| `Scenarios/` | Scenario Harness |
| `docs/` | Canone e documentazione |
| `tools/` | Validator/generator |
| `scripts/rt-suite.ps1` | Suite locale |

Mappa dettagliata:

[`docs/technical/architecture/architettura-codice.md`](docs/technical/architecture/architettura-codice.md)