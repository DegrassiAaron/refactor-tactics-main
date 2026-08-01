# CLAUDE.md — RefactorTactics

Guida operativa per Claude Code / SuperClaude in questo repository.
Obiettivo: modifiche **piccole, verificabili, coerenti col piano canonico e sostenibili** —
non tanto codice in fretta.

## Cos'è il progetto

**RefactorTactics** — gioco tattico PvP a **turni simultanei** (ispirato ad *Atlas Reactor*),
sviluppato come **percorso didattico per imparare Unreal Engine 5.8** partendo da un profilo C#.
Loop: **pianificazione simultanea** → risoluzione a fasi **Prep → Dash → Blast → Move**
(calcolate simultaneamente, applicate in ordine deterministico).

> ⚠️ È presente solo lo **scheletro C++** del progetto (`.uproject`, modulo, `Config/`). Le classi di
> gioco si creano **per milestone**, non in anticipo.

## Fonte di verità (in ordine di autorità)

1. **`docs/design/piano-canonico-mvp.md`** — decisioni operative vincolanti dell'MVP. Prevale su tutto.
2. **`docs/design/roadmap-checkpoint.md`** — milestone, checkpoint, Definition of Done misurabili.
3. Issue/task corrente · specifica di feature · ADR · test esistenti · implementazione corrente.
4. I PDF in `docs/` (3 PRD + `Intenti condivisi` + `…piano completo di sviluppo`) = **visione north-star**, non scope MVP.
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
9. Costruisci **solo** ciò che serve all'MVP; le feature north-star restano fuori scope finché l'MVP non è chiuso.

## Decisioni tecniche fissate

- **Motore**: Unreal Engine **5.8.1** (bloccata; non aggiornare salvo bug bloccanti).
- **Linguaggi**: **regole/dati/resolver/test in C++**, **presentazione/UI/VFX/camera/input in Blueprint**.
- **C# non è il runtime**: l'utente viene da C# e impara C++. Spiega C++ con confronti mirati a C#, **non**
  convertire il progetto in C#, **non** aggiungere UnrealCLR/UnrealSharp/runtime managed per supposizione
  (richiederebbe un ADR esplicito).
- **No GAS nell'MVP**: abilità via `URTAbilityData : UPrimaryDataAsset`. GAS è post-MVP.
- **Nome/prefissi**: progetto `RefactorTactics`; classi con prefisso **`RT`/`URT`** (non `AT`/`UAT`).
- **Scope MVP**: **2v2 offline contro bot**. Multiplayer rimandato, ma architettura *server-authority-ready*.
- **VCS**: Git + **Git LFS** (asset binari UE via `.gitattributes`).
- Il progetto UE vive nella **radice del repo** (`RefactorTactics.uproject`, `Source/`, `Content/`, `Config/`).

## Invarianti architetturali (non negoziabili)

1. **Le regole decidono l'esito** (C++); animazioni/VFX non decidono nulla.
2. Posizione autorevole = **griglia logica** `FRTGridCoord`; il `FVector` serve solo al rendering.
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

- **Documentazione** sempre in `docs/` (sottocartella pertinente, es. `docs/design/`). Mai a radice del progetto.
- **Classi**: prefissi `RT`/`URT`; `PascalCase`; header minimali; `UPROPERTY`/`UFUNCTION` solo quando servono.
- **Asset UE**: `BP_ WBP_ BPI_ DA_ DT_ IA_ IMC_ L_ M_ MI_ T_ NS_ S_` (vedi piano canonico §5).
- **Non versionare**: `Binaries/ DerivedDataCache/ Intermediate/ Saved/ .vs/`, file generati IDE, segreti.
  Non modificare a mano `.uasset`/`.umap`.
- Quando serve l'Editor UE: descrivi i passi esatti (asset, proprietà) + una verifica finale; **non fingere**
  di aver completato una modifica su file binari.

## Tutoring C++ per sviluppatore C#

Quando introduci C++: spiega lifetime/ownership, pointer/reference/validità, reflection Unreal e macro,
differenze GC Unreal vs .NET, thread/authority. Formato: *cosa costruiamo → concetto Unreal/C++ → differenza
da C# → file coinvolti → implementazione → come provarla → errori comuni*.

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
- Catalogo completo e scelta rapida: **`docs/SuperClaude_RefactorTactics_CheatSheet.md`**.

## Formato risposte

- **Prima di implementare**: Obiettivo · Stato verificato · Assunzioni · File coinvolti · Approccio · Rischi · Test previsti.
- **Dopo**: Risultato · File modificati · Decisioni · Test/Build eseguite · Verifiche manuali · Limiti aperti · Prossimo passo.
- Non dichiarare "funziona / completo / production ready / sicuro / deterministico" **senza evidenza**.

## Lingua

Rispondi e commenta **in italiano**. Termini tecnici e identificatori di codice restano in inglese.

## Come lavorare qui

Prima di implementare, **rileggi `docs/design/piano-canonico-mvp.md`**. Costruisci per milestone
(`docs/design/roadmap-checkpoint.md`): M0 Fondamenta → M1 Sandbox → M2 Turn loop → M3 Combat loop →
M4 Vertical slice → M5 Release interna.
