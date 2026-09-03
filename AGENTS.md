# AGENTS.md — RefactorTactics

Contratto operativo condiviso per coding agent nel repository.

Obiettivo: modifiche **piccole, verificabili, coerenti con gli owner correnti e con la milestone attiva**.

> Numeri di test, SHA, issue aperte, checkpoint e altri dati volatili si misurano sul branch corrente. Non copiarli qui.

## 1. Progetto in 30 secondi

**RefactorTactics** è un tattico competitivo a turni simultanei in **Unreal Engine 5.8.1**.

- **v0.1:** 2v2 offline contro bot.
- **Standard:** 3v3 — D-256.
- **2v2:** Skirmish / vertical slice.
- **4v4+:** Operations / stress e scala.
- **Mappa:** grafo esagonale multilivello.
- **Coordinate:** `FRTCellId{X=q, Y=r, Layer}`.
- **Roster v0.1:** Gadget · Phase · Riktor · Wraith.
- **Ability system v0.1:** `UPrimaryDataAsset`.
- **GAS:** fuori scope v0.1.

Loop:

`Planning → Prep → Dash → Blast → Move → Cleanup`

Il Move normale resta l'ultima fase volontaria.

Azioni universali:

`Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`

## 2. Fonti da leggere

Non lavorare dalla memoria del progetto.

Quando pertinenti:

1. `docs/product/piano-canonico-mvp.md`
2. `docs/decisions/RT_PDR_00_Decision_Log.md`
3. ADR applicabili
4. `docs/DOC_CONFLICT_MATRIX.md`
5. `docs/OPEN_DECISIONS.md`
6. `docs/roadmap/roadmap-checkpoint.md`
7. `docs/roadmap/roadmap-v0.1.md`
8. issue/task corrente
9. spec owner
10. cataloghi applicabili
11. codice
12. test

### Non sono source of truth per default

- `docs/research/`
- `docs/archive/`
- PDF/export
- handoff
- audit
- vecchi snapshot
- conteggi copiati

Un `.pdf` è reference/export/audit artifact, non owner normativo: **D-009**.

Il vecchio `feature-registry.yaml`, shortlists e viste derivate sono stati rimossi con **D-181**. Non ricrearli implicitamente.

Se due fonti normative sono incompatibili:

**non scegliere per plausibilità.**

Individua l'owner, segnala la deriva e aggiorna conflict matrix/owner quando il task lo richiede.

## 3. Invarianti

1. **La simulazione decide, la presentazione mostra.**
2. Snapshot + regole/versione + seed + decisioni registrate devono poter riprodurre lo stesso risultato.
3. Ordinamenti competitivi sempre espliciti.
4. Non dipendere dall'ordine di `TMap`/`TSet`.
5. Non usare frame rate o arrivo dei pacchetti per decidere un esito.
6. Niente `DeltaTime`, `Delay`, timeline o callback di animazione nel resolver competitivo.
7. Posizione gameplay = `FRTCellId`.
8. World transform = presentazione.
9. Un solo substrato spaziale: niente seconda griglia quadrata.
10. Non creare un Actor per ogni cella/esagono.
11. Pathfinding, LOS, targeting e traiettorie sono servizi distinti.
12. C++ definisce cosa è possibile.
13. Data Asset/Blueprint configurano varianti e presentazione.
14. ID, costi, priorità, duration, reason code e formati serializzati competitivi sono espliciti/versionati.
15. Un'abilità ha un solo owner.
16. Niente `PairBonus`, `ComboAbility` o branch core `if HeroA && HeroB` per sinergie sistemiche.
17. Scenari, fazioni e Wiki non sono una seconda fonte di numeri competitivi.

## 4. Authority e privacy

Il client propone. L'autorità valida e applica.

- Il server può conoscere lo snapshot completo.
- Il client riceve solo informazioni autorizzate.
- Non replicare globalmente intenti privati per poi nasconderli in UI.
- Nessun planning avversario in `GameState`.
- Nessun planning avversario in `PlayerState`.
- Nessun planning avversario su Actor AlwaysRelevant.
- Nessun planning avversario nel log pubblico prima del momento autorizzato.
- UI e warning usano stato pubblico, Team Knowledge e intenti della propria squadra.
- Overwatch e reazioni non leggono intenti privati o trigger futuri.

## 5. Movimento e reazioni

Famiglie di movimento:

### Traversal

Percorre lo spazio.

`Move · Dash · Forced`

Produce una sequenza di celle attraversate.

Ogni cella può generare fatti:

- hazard;
- trigger;
- attraversamento bordo;
- occupazione;
- interazioni.

### Transfer

Cambia posizione senza percorrere celle intermedie.

L'owner gameplay corrente stabilisce quali azioni appartengono alla famiglia.

### Non confondere

`Reaction` è una causa, non una famiglia di movimento.

`Portal` è topologia del grafo.

### Pin

- Sprint = profilo Move.
- Sprint ≠ Dash.
- Overwatch è universale.
- Overwatch compete con l'azione offensiva principale.
- Delayed/Predictive Action viene scelta nel Planning.
- Fast Action continua una propria azione.
- Fast Reaction deriva da un trigger esterno.
- Modello live: `Opportunity → Commit`.
- Fast Reaction baseline: **3,0 s**.
- Timeout: **HOLD**.
- Thin slice Predictive v0.1: `Hero.Wraith.InterceptShot`.
- High Ground non dà automaticamente `+Damage` o `+VisionRange`.

## 6. Repository

| Percorso | Responsabilità |
|---|---|
| `Source/RefactorTactics/` | Runtime C++ e Automation Tests |
| `Source/RefactorTacticsEditor/` | Tooling Editor-only |
| `Plugins/RTDeveloperTools/` | Developer tooling |
| `Content/RT/` | Asset proprietari `/Game/RT/` |
| `Scenarios/` | Scenario Harness JSON |
| `Config/` | Config Unreal |
| `docs/` | Canone e documentazione |
| `tools/` | Validatori e generatori |
| `scripts/rt-suite.ps1` | Suite Unreal locale |

Mappa dettagliata:

`docs/technical/architecture/architettura-codice.md`

## 7. Asset Unreal

- Prefissi C++: `RT` / `URT`.
- Asset proprietari: `/Game/RT/`.
- Struttura Content: feature-first.
- Non modificare `.uasset`/`.umap` a mano.
- Non spostare asset Unreal da Explorer/filesystem.
- Usare Content Browser.
- Dopo rename/spostamenti: Fix Up Redirectors.
- I binari Unreal non sono mergeabili.
- Un asset binario viene modificato da un solo lavoro per volta.
- Rispettare Binary Asset Lease e write-set.
- Non sovrascrivere lavoro non correlato.
- Il repository non usa Git LFS.
- Le eccezioni di asset tracciati seguono la policy corrente del repository.
- Non versionare `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `.vs/` o segreti.
- Non editare viste generate: modifica sorgente/generatore e rigenera.

Owner:

`docs/technical/tooling/convenzioni-contenuti-ue.md`

## 8. Protocollo di lavoro

### Prima

Annota:

**obiettivo · branch/HEAD · working tree · owner · assunzioni · write-set · approccio · rischi · test**

Preflight:

```bash
git status
git branch --show-current
git fetch --prune origin
git rev-parse HEAD
git rev-parse origin/main
```

Poi:

- cerca prima di creare;
- verifica i path reali;
- verifica le API UE 5.8.1;
- riusa classi/helper/spec/test esistenti;
- non espandere lo scope;
- evita refactor collaterali;
- controlla riferimenti prima di cancellare o spostare.

### Durante

- Mantieni piccolo il write-set.
- Non creare una seconda source of truth.
- Non fare search/replace ciechi sugli Stable ID.
- Per formati serializzati prevedi migrazione/compatibilità quando richiesta.
- Una feature presentation-only non modifica stato competitivo senza requisito esplicito.
- Se un gate non è stato eseguito: **NOT RUN**.

### Dopo

Riporta:

**risultato · file · decisioni · build/test · verifiche manuali · NOT RUN · limiti · prossimo passo**

Non dichiarare:

- completo;
- production ready;
- sicuro;
- deterministico;
- verificato;

senza evidenza.

## 9. Build e test

Non esiste CI automatica per scelta corrente.

Non introdurre CI, package manager o nuovi build step senza una decisione esplicita.

### Suite Unreal

Da PowerShell:

```powershell
./scripts/rt-suite.ps1
./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario
./scripts/rt-suite.ps1 -WaitMinutes 40
```

Una misura è valida soltanto se osserva lo stesso:

- `HEAD`;
- working tree;
- binario;
- stato del motore;

dall'inizio alla fine.

Se cambiano:

**NON VALIDA**.

Non equivale a verde.

Non sostituire `-WaitMinutes` con watcher scritti a mano.

Dopo una lunga attesa, ricompila prima di registrare il risultato: il DLL presente sul disco potrebbe provenire da un'altra sessione.

Prima del merge verifica che il gate appartenga al commit che stai mergiando.

### Build Editor

Con Unreal Editor chiuso:

```powershell
& "<engine>/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development `
    -project="<repo>/RefactorTactics.uproject" -waitmutex
```

### Tooling locale

Controlla sempre il contenuto corrente di `tools/`.

Controlli noti:

```powershell
node tools/radar/generate.ts --check
node tools/radar/wiki-alt.ts --wiki-root <clone> --check
node tools/radar/doc-links.ts --check
node tools/radar/catalog-code.ts
node tools/radar/doc-tables.ts --check
node tools/radar/issue-refs.ts --check
node tools/radar/scenario-notes.ts --check
node tools/asset-refs/check.ts
python tools/architettura/misure-strutturali.py --check   # solo se la PR tocca Turn/RTTurnManager.*

cd tools/radar
node --test
```

Ogni tool dichiara nel docstring **cosa non copre**.

⛔ `tools/mutation/costanti-combattimento.py` **non e' fra i controlli noti**, e di proposito. Modifica un sorgente e occupa il motore per **un build completo piu' una suite intera per ogni costante**, piu' una baseline: con le 11 di `RTCombatLibrary.h` sono **ore**, e le direzioni di mutazione da misurare sono **due** (`+3` e `-3` danno risposte diverse — vedi il docstring). Mentre gira, ogni altra misura in parallelo e' NON VALIDA. Si lancia per rispondere alla domanda che `#2118` ha posto — *quali costanti si possono cambiare senza che niente diventi rosso* — non a ogni PR.

⚠️ E se non stampa `AUDIT COMPLETO`, **ricostruire prima di qualunque altra misura**: un'interruzione lascia mutato anche il binario, che e' la meta' che `rt-suite` non sa vedere.

⚠️ `scenario-notes.ts` confronta i numeri citati nella **prosa** di uno scenario con ciò che il file
stesso asserisce — è la deriva che `#1904` ha misurato propagarsi nei documenti a valle, e che `#2049`
ha ripulito. **Ordina, non decide**: ogni riga segnalata va letta, e un verde non è una prova di
assenza. Le tre cose che non vede sono nel suo docstring.

Un verde dimostra soltanto ciò che quel tool misura.

### `issue-refs.ts` — l'unico che guarda fuori dal repository

Confronta i percorsi e i comandi citati dalle **issue aperte** con l'albero: chiude il difetto che
`doc-links.ts` dichiara di non coprire, cioè i riferimenti scritti in prosa dove nessun link li rende
verificabili.

Tre cose da sapere prima di usarlo:

- **Segnala il cancellato, non l'assente.** Un percorso mai esistito è un deliverable e non è un
  difetto; uno rimosso è un riferimento morto. La distinzione toglie ~114 falsi positivi.
- **Serve la storia completa.** Su un clone shallow dichiara `NOT RUN` invece di un verde: senza
  `git log --diff-filter=D` nessun percorso risulta rimosso.
- **Senza rete dichiara `NOT RUN` ed esce 0.** Non blocca chi lavora offline e non finge un verde.

Una issue il cui *oggetto* è la rimozione si esenta dal proprio corpo, **col motivo obbligatorio**:

```html
<!-- issue-refs: ignora — perché questa issue cita di proposito percorsi rimossi -->
```

Il gate stampa le esenzioni a ogni esecuzione.

⚠️ **Si lancia a mano, come gli altri radar** — vedi §9: *«non introdurre CI senza una decisione
esplicita»*. Il difetto che chiude però non nasce da un commit, **nasce dal tempo che passa** fra la
rimozione di un percorso e la issue che nessuno riapre.

Per questo `rt-suite.ps1` lo esegue **come promemoria** dopo un verdetto `VALIDA`, e lo si disattiva con
`-NoIssueRefs`:

🔴 **Non concorre al verdetto della suite, ed è una scelta, non una svista.** Legge GitHub, che cambia
mentre la suite gira: in una run da quaranta minuti può passare all'avvio e fallire alla fine. Farlo
entrare nelle invarianti di §9 renderebbe `NON VALIDA` una misura sana per una issue che ha modificato
qualcun altro — cioè il difetto che `rt-suite.ps1` esiste per impedire. Stampa, e l'esito resta quello
dei test.

Gli output generati dei radar non si editano a mano.

## 10. Definition of Done

Quando applicabile:

- Game compila.
- Editor compila.
- Test mirati passano.
- Regressione pertinente passa.
- La misura è valida.
- Determinismo preservato.
- Authority preservata.
- Privacy preservata.
- TurnLog/reason code spiegano l'esito.
- Owner documentale aggiornato.
- Nessun secret introdotto.
- Nessun output locale indesiderato.
- PIE verificato quando richiesto.
- Packaged verificato quando richiesto.

## 11. Lavoro parallelo

Il repository viene modificato da più sessioni.

Non assumere stabili:

- `HEAD`;
- `origin/main`;
- working tree;
- DLL;
- issue;
- ID condivisi.

Un worktree separato non elimina il mutex globale Unreal/Live Coding.

Prima del merge rimisura.

## 12. Git

Branch focalizzati:

- `feat/`
- `fix/`
- `refactor/`
- `docs/`
- `test/`

Usare Conventional Commits.

Controllare sempre:

```bash
git status
git diff
```

prima del commit.

Non confondere:

**file modificato**

con

**file verificato**.

### Chiudere una issue

`fix(605)` **non chiude** la issue 605.

GitHub lo legge come uno **scope Conventional Commits**, non come un riferimento. La forma che chiude e' una
parola chiave seguita da `#N`:

```
Closes #605
```

Le parole riconosciute sono `close`/`closes`/`closed`, `fix`/`fixes`/`fixed`,
`resolve`/`resolves`/`resolved`. `fix(605)` non e' nessuna di queste: manca il `#`, e la parentesi ne fa
uno scope.

**Dove va**: nel **corpo della PR**, in cima. Non nel messaggio di commit.

Il corpo della PR e' il canale che GitHub processa al merge, ed e' l'unico che un agente controlla davvero:
`gh pr create --body-file` scrive li'.

⛔ **`.github/pull_request_template.md` non basta.** Il template si applica solo alle PR aperte
dall'interfaccia web: `gh pr create` con `--body` o `--body-file` lo **sostituisce**, e non avvisa. Chi apre
PR da riga di comando — cioe' ogni agente — deve scrivere la riga a mano.

⚠️ **Se la base della PR non e' il branch di default, `Closes` non chiude niente al merge.** La issue si
chiudera' solo quando quel branch arrivera' su `main`. Una PR verso un branch padre intermedio chiude la
propria issue **a mano**, oppure lo dichiara.

**Perche' questa sezione esiste.** Misurato il 2026-09-02: **57** issue aperte avevano almeno un commit
`fix()`/`feat()` mergiato su `main`, e nessuna si era chiusa da sola. Fra queste, `#1473`, `#605`, `#75` e
`#61` avevano il lavoro **finito**: la loro correzione era su `main` da giorni o settimane, e restavano
aperte perche' il messaggio diceva `fix(1473)` invece di `fixes #1473`.

⚠️ **Non e' un difetto di disciplina, ed e' per questo che vale una regola scritta**: `fix(605)` *sembra* un
riferimento. Il triage completo e' nel commento di chiusura di
[`#1473`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1473).

### ID condivisi

`D-nnn`, Epic `Enn` e altri contatori non si assegnano dalla memoria.

Prima di assegnare o mergiare:

1. `git fetch --prune origin`
2. controlla `main`;
3. controlla ref remoti;
4. controlla PR;
5. controlla issue;
6. riverifica immediatamente prima del merge.

Un branch remoto può rivendicare un ID anche senza PR aperta.

In caso di collisione correggi i riferimenti puntuali.

Non fare search/replace globale.

## 13. Lingua

Documentazione e comunicazione:

**italiano**.

Codice, identificatori e API:

**inglese**.

Tutoring C++/UE su richiesta, non come default.