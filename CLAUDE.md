# CLAUDE.md — RefactorTactics

Guida operativa per Claude Code / SuperClaude in questo repository.
Obiettivo: modifiche **piccole, verificabili, coerenti col piano canonico e sostenibili** —
non tanto codice in fretta.

## Cos'è il progetto

**RefactorTactics** — gioco tattico PvP a **turni simultanei** (ispirato ad *Atlas Reactor*) su Unreal
Engine 5.8, dev singolo. Loop: **pianificazione simultanea** → risoluzione a fasi
**Prep → Dash → Blast → Move** (calcolate simultaneamente, applicate in ordine deterministico).
Griglia **esagonale** (`FRTCellId`, assiale/cubica) con editor mappa data-driven.

> **Fase tutorial chiusa (2026-08-05)**: il progetto è nato come percorso didattico UE partendo da C# e ha
> prodotto l'MVP quadrato M0–M5, ora archiviato. Da qui in poi è un progetto di **prodotto**: si costruisce
> per milestone M6+ (vedi roadmap), non per lezioni.

## Fonte di verità (in ordine di autorità)

1. **`docs/product/piano-canonico-mvp.md`** — decisioni operative vincolanti (invarianti, architettura, regole). Prevale su tutto.
2. **`docs/roadmap/roadmap-checkpoint.md`** — milestone, checkpoint, Definition of Done misurabili, **stato**.
3. Issue/task corrente · specifica di feature · ADR · test esistenti · implementazione corrente.
4. I PDF in `docs/src/` (3 PRD + `Intenti condivisi` + `…piano completo di sviluppo`) = **visione north-star**, non scope corrente.
5. Questo file (`CLAUDE.md`).

**Se due fonti sono in conflitto**: non scegliere in silenzio → segnala il conflitto, indica l'impatto,
proponi la modifica minima, non sovrascrivere una decisione approvata senza documentarla.
Materiale superato/non autorevole: `docs/archive/`.

## Regole prioritarie

1. Leggi il contesto del repo **prima** di proporre modifiche; cerca implementazioni/convenzioni esistenti.
2. Non duplicare classi, componenti, documenti o convenzioni. Non inventare requisiti, API, metriche o dipendenze.
3. Se un requisito è ambiguo, separa: **fatti verificati / assunzioni / decisioni richieste / raccomandazioni**.
4. Prima di una feature complessa, prepara o aggiorna specifica e design.
5. Non dichiarare un lavoro completo senza le verifiche applicabili (build + test).
6. Niente commit, push, merge o operazioni remote/distruttive senza richiesta esplicita.
7. Non eliminare codice/asset/dati senza verificare riferimenti (anche Blueprint/reflection).
8. Preferisci modifiche piccole e revisionabili a grandi riscritture.
9. Costruisci **solo** ciò che serve alla milestone corrente della roadmap; le feature north-star restano fuori
   scope finché la milestone non è chiusa.

## Decisioni tecniche fissate

- **Motore**: Unreal Engine **5.8.1** (bloccata; non aggiornare salvo bug bloccanti).
- **Linguaggi**: **regole/dati/resolver/test in C++**, **presentazione/UI/VFX/camera/input in Blueprint**.
- **C# non è il runtime**: **non** convertire il progetto in C#, **non** aggiungere UnrealCLR/UnrealSharp o
  runtime managed per supposizione (richiederebbe un ADR esplicito).
- **No GAS**: abilità via **`URTActionData : UPrimaryDataAsset`** (+ `URTHeroData`, `URTEquipmentData`).
  GAS resta north-star (il PDR lo prevede in F2,
  prevale il canone).
- **Nome/prefissi**: progetto `RefactorTactics`; classi con prefisso **`RT`/`URT`** (non `AT`/`UAT`).
- **Scope corrente**: **2v2 offline contro bot** su griglia esagonale. Multiplayer pianificato in **M10**,
  architettura *server-authority-ready* fin d'ora.
- **VCS**: Git + **Git LFS** (asset binari UE via `.gitattributes`).
- Il progetto UE vive nella **radice del repo** (`RefactorTactics.uproject`, `Source/`, `Content/`, `Config/`).

## Invarianti architetturali (non negoziabili)

1. **Le regole decidono l'esito** (C++); animazioni/VFX non decidono nulla.
2. Posizione autorevole = **cella logica** — `FRTCellId` (esagonale, assiale/cubica) è il target; `FRTGridCoord`
   (quadrata) è il residuo in dismissione (M7). Il `FVector` serve solo al rendering.
3. Resolver **"raccogli poi applica"**: snapshot a inizio fase, niente `Delay`/timeline/montage nel resolver,
   l'ordine dell'array non deve cambiare l'esito.
4. **Determinismo**: niente `DeltaTime` non controllato nella logica dei turni; niente dipendenza dall'ordine
   di container non ordinati; ogni RNG futuro usa seed/stream espliciti; ogni formato serializzato è versionato.
5. **Server autoritativo** per ogni decisione di gameplay; il client calcola solo preview.
6. **Privacy dell'intento**: le intenzioni di pianificazione degli alleati **non** devono mai raggiungere i
   client avversari — niente replica globale + occultamento grafico; usa stato server + replica filtrata per
   squadra + autorizzazione server-side.
7. **Combat math = funzioni pure** testabili (`URTCombatLibrary`).

## Pilastri di prodotto

Leggibilità tattica · predizione (non RNG opaco) · coordinazione di squadra · mappa come sistema di gioco ·
identità dei personaggi con scelta orizzontale (nessuna potenza permanente pay-to-win) · determinismo e
verificabilità · estendibilità controllata (dati/regole espandibili senza core pieno di eccezioni hard-coded).

## Blueprint o C++

- **Blueprint** quando: iterazione rapida di design · comportamento di presentazione · contenuto per designer.
- **C++** quando: simulazione autorevole · determinismo · logica condivisa · rete/performance · test robusti.
- Non spostare automaticamente tutto in C++ o tutto in Blueprint.

## Convenzioni

- **Documentazione** sempre in `docs/` (sottocartella pertinente, es. `docs/gameplay/`, `docs/technical/`; indice in `docs/README.md`). Mai a radice del progetto.
- **Classi**: prefissi `RT`/`URT`; `PascalCase`; header minimali; `UPROPERTY`/`UFUNCTION` solo quando servono.
- **Asset UE**: tutto il proprietario sotto **`/Game/RT/`**, organizzato **feature-first** (mai cartelle globali
  per tipo tipo `Blueprints/`, `Materials/`, `Meshes/`); naming `<Tipo>_<Feature>_<Nome>` con prefissi
  `BP_ BPC_ WBP_ ABP_ DA_ DT_ Curve_ SM_ SK_ M_ MI_ T_ NS_ SFX_ MUS_ L_ IMC_ IA_`. Terze parti (Paragon,
  Marketplace) restano fuori da `/Game/RT`. Regole complete, dipendenze consentite e procedura di spostamento:
  **[`docs/technical/convenzioni-contenuti-ue.md`](docs/technical/convenzioni-contenuti-ue.md)** — vincolante.
- Gli `.uasset`/`.umap` si spostano **dal Content Browser**, mai da Esplora File; dopo lo spostamento aggiorna i
  percorsi hard-coded in `Config/*.ini` e C++, poi `Fix Up Redirectors`.
- **Non versionare**: `Binaries/ DerivedDataCache/ Intermediate/ Saved/ .vs/`, file generati IDE, segreti.
  Non modificare a mano `.uasset`/`.umap`.
- Quando serve l'Editor UE: descrivi i passi esatti (asset, proprietà) + una verifica finale; **non fingere**
  di aver completato una modifica su file binari.

## Spiegazioni C++/UE — su richiesta

Con la chiusura della fase tutorial il tutoring **non è più il default**: vai al punto, spiega il codice che
scrivi, non il linguaggio. Se l'utente lo chiede («spiegami», «perché così»), usa il formato completo:
*cosa costruiamo → concetto Unreal/C++ → differenza da C# → file coinvolti → implementazione → come provarla →
errori comuni*, con attenzione a lifetime/ownership, validità dei puntatori, reflection Unreal, GC vs .NET,
thread/authority.

## Test & Definition of Done

- **Priorità test**: resolver · ordine fasi · conflitti movimento · pathfinding/cost provider · LOS/cover ·
  validazione ordini · privacy dei piani · determinismo · serializzazione.
- **Strumenti**: Unreal Automation Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`), eseguibili da Editor e CLI.
- **DoD** (elementi applicabili): requisiti aggiornati · compila · test automatici + regressione · verifica
  authority/privacy/determinismo dove pertinente · nessun segreto/file generato · nessun warning nuovo non
  spiegato · documentazione aggiornata · limiti dichiarati. Se qualcosa non è verificabile, **dichiaralo**.

## Git

- Repository: `DegrassiAaron/refactor-tactics-main` (owner **DegrassiAaron**).
- Il push HTTPS richiede l'account gh **`DegrassiAaron` attivo** (tende a tornare a `meepleAi-app`, che dà 403);
  vedi la memoria di progetto per il workaround al blocco di Git Credential Manager.
- Branch di feature per ogni lavoro (`feat/… fix/… refactor/… docs/… test/…`); Conventional Commits;
  status+diff prima del commit; niente file generati/segreti; PR verso il branch padre.

## SuperClaude: documentali vs esecutivi

- **Documentali** (non modificano codice — fermati dopo l'output): `/sc:brainstorm /sc:research /sc:design
  /sc:workflow /sc:spawn /sc:analyze /sc:estimate /sc:spec-panel /sc:business-panel /sc:troubleshoot` (senza `--fix`).
- **Esecutivi** (possono modificare): `/sc:implement /sc:task /sc:improve /sc:cleanup /sc:test /sc:build
  /sc:git /sc:troubleshoot --fix`.
- **Non** passare in automatico da documentazione a esecuzione: un comando documentale non autorizza modifiche al codice.
- Catalogo completo e scelta rapida: **[`docs/src/SuperClaude_RefactorTactics_CheatSheet.md`](docs/src/SuperClaude_RefactorTactics_CheatSheet.md)**.

## Formato risposte

- **Prima di implementare**: Obiettivo · Stato verificato · Assunzioni · File coinvolti · Approccio · Rischi · Test previsti.
- **Dopo**: Risultato · File modificati · Decisioni · Test/Build eseguite · Verifiche manuali · Limiti aperti · Prossimo passo.
- Non dichiarare "funziona / completo / production ready / sicuro / deterministico" **senza evidenza**.

## Lingua

Rispondi e commenta **in italiano**. Termini tecnici e identificatori di codice restano in inglese.

## Come lavorare qui

Prima di implementare, **rileggi `docs/product/piano-canonico-mvp.md`** (decisioni) e
**`docs/roadmap/roadmap-checkpoint.md`** (milestone corrente e DoD).

Milestone attive: **M6 Parità hex** (la partita passa sulla griglia esagonale) → **M7 Dismissione del
quadrato** → **M8 Presentazione** → **M9 Ambienti/editor** → **M10 Rete e privacy** → **M11 Production
readiness**. M0–M5 (MVP quadrato, fase tutorial) e H0–H6.5 (fondamenta esagonali) sono **chiuse**.
