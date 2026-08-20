# AGENTS.md — RefactorTactics

Guida operativa condivisa per agenti di coding nel repository.
Obiettivo: cambi **piccoli, verificabili, coerenti con le decisioni correnti e con la milestone attiva**.

## Progetto in 30 secondi

**RefactorTactics** è un tattico competitivo a turni simultanei in **Unreal Engine 5.8.1**.
La v0.1 è un vertical slice **2v2 offline contro bot** su griglia **esagonale multilivello**.

Loop canonico:

`Planning → Prep → Dash → Blast → Move → Cleanup`

Il **Move normale resta l'ultima fase volontaria**. Dash e spostamenti speciali possono avvenire prima solo
perché appartengono alla loro fase/regola specifica.

Roster v0.1 corrente: **Gadget · Phase · Riktor · Wraith** (D-120). I nomi legacy del roster **non esistono più
nel repository** ([D-130](docs/decisions/RT_PDR_00_Decision_Log.md), che ne conserva la mappatura): gli
`Hero.<Nome>` sono stati rinominati e i venti token abilità sono atterrati su `Hero.<Nome>.<Abilità>` **senza
redirect** — [D-134](docs/decisions/RT_PDR_00_Decision_Log.md) ha cancellato `ResolveLegacyActionId`. Le cinque
fette del piano sono chiuse: oggi un nome legacy che ricompare è un difetto, non un residuo.
Il formato competitivo finale non è ancora bloccato: **3v3 è una baseline di studio, 4v4 uno stress test**, non
trasformarli in una decisione definitiva.

## Prima di modificare qualcosa

Carica solo il contesto necessario, ma non saltare queste fonti quando pertinenti:

1. **`docs/product/piano-canonico-mvp.md`** — invarianti e decisioni canoniche.
2. **`docs/decisions/RT_PDR_00_Decision_Log.md`** + ADR applicabili — decisioni esplicite successive.
3. **`docs/DOC_CONFLICT_MATRIX.md`** — cosa è stato superato e cosa è ancora aperto.
4. **`docs/OPEN_DECISIONS.md`** — non decidere al posto del progetto.
5. **`docs/roadmap/roadmap-checkpoint.md`** — stato di esecuzione misurato e milestone M6–M11.
6. **`docs/roadmap/roadmap-v0.1.md`** — scope e gate della release v0.1.
7. **`docs/roadmap/feature-registry.yaml`** — lo stato di una feature vive **qui e in nessun altro posto**.
   `status` è **derivato** dai gate, non dichiarato; roadmap, Wiki e workbook referenziano il `feature_id` e
   leggono lo stato, non lo copiano. Le viste (`feature-registry.json`, `project-graph.json`, le cinque
   `*.shortlist.md`) sono **generate**: si rigenerano, non si editano.
8. Issue/task corrente, specifica di feature, cataloghi in `docs/balance/`, test e codice esistente.

`docs/research/` contiene i sorgenti **non ancora consumati** (PRD di visione, dataset, media): **non è fonte
normativa per default**. *(Era `docs/src/` fino al 2026-08-19: quel nome era ambiguo in un repository che ha
anche `Source/`, e la cartella teneva quattro cose diverse — oggi la ricerca sta in `docs/research/` e gli
output generati in `docs/generated/`,
[#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165).)*
*(Dal 2026-08-12 sono Markdown: in `docs/` non c'è più prosa in formato binario —
[D-009](docs/decisions/RT_PDR_00_Decision_Log.md).)* `docs/archive/` è storico — e dal 2026-08-08 include
[`docs/archive/src/`](docs/archive/src/README.md), dove finiscono i sorgenti **già recepiti** (design, handoff,
audit) con l'indice di chi li possiede oggi. Se cerchi la provenienza di una regola, è lì; se cerchi la regola,
è nell'owner.

### PDF: mai source of truth

Un `.pdf` non è **mai** fonte normativa: è export, reference o audit artifact — inclusi i PRD/PDR esportati
([D-009](docs/decisions/RT_PDR_00_Decision_Log.md)). Non entra nel preflight; si apre solo quando il task
chiede provenienza, rationale storico o confronto con una decisione precedente. Se un PDF è l'export di una
spec Markdown corrente, **il Markdown resta l'owner**: un export è una vista, e una vista segue la sorgente.

⚠️ La sigla `PDR`/`PRD` nel **nome** non rende storico un file Markdown — la sostanza è chi possiede la regola
oggi, non come si chiama il file. [`docs/decisions/RT_PDR_00_Decision_Log.md`](docs/decisions/RT_PDR_00_Decision_Log.md)
è canonico proprio perché è l'owner corrente del Decision Log.

Se una decisione più recente dichiara esplicitamente di superare una regola più vecchia ma il canone non è
ancora sincronizzato, **segnala la deriva e aggiorna gli owner documentali pertinenti**; non scegliere per
plausibilità. Se due fonti normative restano davvero incompatibili, fermati e registra/segnala il conflitto.

## Decisioni tecniche correnti

- **Engine**: Unreal Engine **5.8.1**, bloccata.
- **Runtime gameplay**: C++ per regole, resolver, dati logici, pathfinding, validazione, serializzazione e test.
- **Presentazione**: Blueprint/UMG/VFX/animazioni/camera/input dove conviene iterare velocemente.
- **No GAS nella v0.1**: azioni e personaggi sono data-driven con `UPrimaryDataAsset` (`URTActionData`,
  `URTHeroData`, `URTEquipmentData`). GAS resta eventuale evoluzione, non introdurlo implicitamente.
- **Coordinate autorevoli**: `FRTCellId{X=q, Y=r, Layer}`. Il vecchio substrato quadrato è rimosso; non
  reintrodurre `FRTGridCoord` o una seconda simulazione.
- **Mappa**: grafo tattico esagonale multilivello; celle/archi sono dati, non migliaia di Actor.
- **Pathfinding**: A* sul grafo, costi interi; LOS, targeting e traiettorie sono servizi distinti.
- **Authority**: gameplay progettato server-authoritative anche quando la v0.1 gira offline.
- **Privacy**: intenti completi solo dove autorizzati; mai replica globale di planning da “nascondere” in UI.
- **VCS**: Git + Git LFS; asset UE binari gestiti dal Content Browser.

## Modello azioni e turni

Le macro-fasi non si cambiano per adattarsi a una singola abilità.

Azioni generiche correnti:

`Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`

Il movimento si divide in **due famiglie**, e la linea non è la velocità
([D-118](docs/decisions/RT_PDR_00_Decision_Log.md), 2026-08-12):

> **`Traversal` percorre lo spazio. `Transfer` cambia posizione senza percorrerlo.**

`Traversal` = `Move · Dash · Forced`: produce una **sequenza di celle percorse**, e ogni cella è un fatto —
hazard, trigger, bordo attraversato, occupazione. `Transfer` = `Leap` oggi, `Blink`/`Swap`/`Recall` in
**E39** (v0.2): esistono un'origine e una destinazione, e nient'altro. La riga che le separa **si verifica in
codice**, ed è cosa contiene `Result.Entered`. `Reaction` non è una famiglia: è una **causa**. `Portal` non è
un `Transfer`: è topologia del grafo. Owner:
[`docs/gameplay/spec-tassonomia-movimento.md`](docs/gameplay/spec-tassonomia-movimento.md).

Regole da non regredire:

- Sette voci, non sei: `Guard` **è tornata universale** e `Activate` resta assorbita da `Interact`
  ([D-025](docs/decisions/RT_PDR_00_Decision_Log.md)). `Guard` aveva già tre consumatori — catalogo azioni,
  `Status.Root`, difesa direzionale di ADR-0005 §4a — quindi non è una stance opzionale.
- **Sprint è un profilo della famiglia Move, non un Dash**.
- Overwatch è un'azione universale di Planning; il comportamento concreto dipende da eroe/equipaggiamento e
  compete con l'azione offensiva.
- Una **Delayed/Predictive Action** è decisa nel Planning e risolve a un boundary dichiarato.
- Una **Fast Action** è una scelta live limitata che continua una propria azione.
- Una **Fast Reaction** è una scelta live causata da un trigger esterno.
- Le finestre live sono **in scope**: modello unificato `Opportunity → Commit`, con decision boundary espliciti.
- Baseline Fast Reaction: **3,0 s**, `Timeout → HOLD`.
- Overwatch non deve conoscere trigger futuri o intenti privati avversari.
- Thin slice predittivo v0.1: **`Hero.Wraith.InterceptShot`**.
- In caso di Intercept, la geometria/cover va rivalidata sul **bersaglio effettivo**, senza aprire una nuova
  opportunity solo per quella rivalidazione.
- High Ground non dà un bonus numerico alla vista nella v0.1: quota, LOS e cover bastano finché i playtest non
  dimostrano il contrario.

## Invarianti architetturali

1. **La simulazione decide, la presentazione mostra.** Animazioni, montage, VFX e frame rate non decidono esiti.
2. **Snapshot + regole/versione + seed + decisioni registrate ⇒ stesso risultato**.
3. Resolver deterministico: ordinamenti espliciti; mai affidarsi all'ordine di `TMap`/`TSet` o all'arrivo dei
   pacchetti.
4. Niente `DeltaTime` o timer real-time nella logica competitiva; le finestre live fermano il resolver a un
   decision boundary e registrano la risposta come input.
5. Il resolver non usa `Delay`, timeline o callback di animazione per stabilire l'ordine logico.
6. Le trasformazioni world (`FVector`) sono presentazione; la posizione gameplay resta la cella logica.
7. Combat math e regole riusabili preferibilmente in funzioni pure/testabili, senza branch per eroe nel core.
8. C++ definisce cosa è possibile; Data Asset/Blueprint scelgono varianti e presentazione.
9. ID, priorità, costi, durata, reason code e formati serializzati che incidono sulla simulazione sono stabili,
   espliciti e versionati.
10. **Un'abilità ha un solo owner: non modellare una sinergia come ability di coppia.** Nessun `PairBonus`,
    `FactionSetBonus` o `ComboAbility` condivisa, e nessun branch `if HeroA && HeroB` per un payoff che le
    regole sistemiche già esprimono.
11. **Producer e consumer comunicano tramite stato/tag/evento/superficie quando la regola è sistemica.** Chi
    applica pubblica lo stato; chi legge dipende dallo stato, non dall'identità di chi l'ha applicato — salvo
    requisito dichiarato *nella* Ability Definition, con counterplay e test propri.
12. **Scenari, fazioni e Wiki non sono fonti competitive.** Dimostrano e spiegano; i numeri restano nei
    cataloghi `docs/balance/`, le abilità nella pagina/definizione del proprio owner. Owner della regola:
    [D-029](docs/decisions/RT_PDR_00_Decision_Log.md) ·
    [ADR-0006](docs/decisions/adr-0006-ownership-abilita-sinergie.md) ·
    [`docs/gameplay/spec-ownership-abilita-interazioni-sinergie.md`](docs/gameplay/spec-ownership-abilita-interazioni-sinergie.md).

## Conoscenza parziale e rete

Non trattare la conoscenza parziale come una Fog of War classica: la mappa statica è nota, mentre la squadra
possiede livelli di conoscenza su unità/eventi.

- Vista e udito sono canali distinti; il rumore è informazione, non un semplice debuff.
- Facing/orientamento influenza percezione, difesa e reazioni dove previsto.
- UI e warning usano solo stato pubblico, Team Knowledge e intenti della propria squadra.
- Il server può conoscere lo snapshot completo; il client riceve solo DTO/informazioni autorizzate.
- Nessun planning avversario in `GameState`, `PlayerState`, Actor AlwaysRelevant o log pubblico prematuro.

## Unreal / contenuti

- Prefissi C++: `RT` / `URT`; `PascalCase`; reflection (`UPROPERTY`, `UFUNCTION`) solo quando necessaria.
- Asset proprietari sotto **`/Game/RT/`**, struttura **feature-first**.
- Naming e dipendenze contenuti: **`docs/technical/tooling/convenzioni-contenuti-ue.md`** è normativo.
- Terze parti/Paragon restano fuori da `/Game/RT` salvo pipeline esplicitamente documentata.
- Non modificare `.uasset`/`.umap` a mano e non spostarli da filesystem: usare Content Browser + Fix Up Redirectors.
- I binari Unreal sono **human-first, non human-only** ([D-139](docs/decisions/RT_PDR_00_Decision_Log.md)):
  l'autore davanti all'Editor è l'holder predefinito, e una sessione Claude li tocca solo con una **Binary
  Asset Lease** esclusiva dichiarata nel batch. Un holder per path, e due `.uasset` **non si fondono**: un
  conflitto binario è una delle due versioni da rifare a mano dentro Unreal.
- Non versionare `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `.vs/`, segreti o output locali.

## Metodo di lavoro

Prima di implementare:

**Obiettivo · stato verificato · assunzioni · file coinvolti · approccio minimo · rischi · test previsti**.

Durante:

- cerca prima di creare: niente classi/spec/helper duplicati;
- non espandere lo scope oltre la milestone/checkpoint corrente;
- evita grandi refactor “già che ci siamo”;
- non inventare API Unreal: verifica la firma disponibile nella 5.8.1/progetto;
- non eliminare codice/asset senza verificare riferimenti C++, config, soft reference e Blueprint;
- per migrazioni di Stable ID o formati serializzati, prevedi compatibilità/validator/test espliciti;
- un documento di handoff/audit non autorizza da solo a implementare tutto ciò che descrive.

Dopo:

**Risultato · file modificati · decisioni · test/build eseguiti · verifiche manuali · limiti aperti · prossimo passo**.

Non scrivere “funziona”, “completo”, “production ready”, “sicuro” o “deterministico” senza evidenza.

## Test e Definition of Done

Priorità: determinismo · resolver/fasi · collisioni e movimento · path/LOS/cover · azioni/reazioni · ambiente ·
validazione · serializzazione/replay · privacy intenti.

- Usare Unreal Automation Framework e test di dominio esistenti.
- Per scenari integrati usare il **RT Scenario Test Harness**: scenario testuale → stessi Intent/Command del gioco
  reale → snapshot/resolver → TurnLog → report machine-readable.
- I test non devono aggirare il gameplay con `SetActorLocation`, `ApplyDamage` o branch `if (IsTest)` che saltano
  la regola sotto test.
- In Fast/Headless niente attese di planning, animazioni o 3 secondi reali: il decision boundary viene risolto
  dalla test policy.
- **Non hardcodare il numero totale dei test nei documenti**: misuralo sul branch/HEAD quando serve.
- **Un numero di test in una PR sono due numeri**: «N eseguiti su M dichiarati». Una run può eseguirne meno
  di quanti ne esistono restando verde (`#486`): verifica con
  `python scripts/feature_registry.py suite --run-log Saved/Logs/RefactorTactics.log`, che esce 1 sui buchi.
  Vale soprattutto prima di dichiarare una **verifica di mutazione**: se il test atteso non era in lista, il
  suo «non è caduto» non significa niente.
- Le verifiche PIE/Editor non sono verdi finché qualcuno non le ha realmente eseguite.
- **Il repository ha due toolchain, e nessuna gira in CI**: Python in `scripts/` e **Node 22** in
  `tools/radar/` — rubrica dei rating e generatore SVG dei radar di personaggio.
  I gate Python sono **ogni `scripts/check-*.py`** (sei il 2026-08-16 — `ls scripts/check-*.py` è
  l'elenco, e non si trascrive qui), più `feature_registry.py validate|generate|shortlist` e
  `rt_shared_id.py check`, che non seguono quel prefisso e vanno nominati.
  *(Riscritto il 2026-08-16: questa riga elencava i gate **per nome** e ne conosceva **tre su sei**.
  Mancavano `check-docs-naming.py`, `check-capability-owners.py` — atterrata con `#1043` lo stesso
  giorno — e `check-equipment-defaults.py`. Non è una svista ripetuta tre volte: un elenco scritto a
  mano dentro un documento non ha modo di accorgersi di un file nuovo, e chi aggiunge un gate non
  passa di qui. La forma `check-*` si aggiorna da sé; il numero accanto è una misura datata, non una
  seconda lista.)*
  I gate si eseguono **a mano**, ed è una scelta:
  **`.github/workflows/` non esiste, e la sua assenza è deliberata.** Non introdurre CI, package manager o
  build step senza chiedere: `tools/radar/` ha **zero dipendenze** apposta.
  *(Corretto il 2026-08-16: la riga diceva «`.github/` non esiste», e dal 2026-08-16 è falsa —
  `.github/ISSUE_TEMPLATE/` esiste e precompila il blocco `## Tracking`. La decisione proteggeva la **CI**,
  non la cartella, e l'oracolo che la verificava — `git ls-tree … -- .github/` in
  [`qa-prompt-terminal-b`](docs/technical/runbooks/qa-prompt-terminal-b-scenario-runner.md) — è stato ristretto a
  `workflows/` nello stesso commit. Un template non esegue niente; un workflow sì. Precedente del
  2026-08-12: diceva «è vuota», che induceva a cercare una cartella inesistente.)*
- ⚠️ **`tools/radar/` non è solo documentazione.** I rating si calcolano dai cataloghi di bilanciamento
  ([D-106](docs/decisions/RT_PDR_00_Decision_Log.md)), quindi cambiare `Salute`, un danno o un cooldown
  rende rossi gli SVG versionati finché non li rigeneri **nello stesso commit**
  ([D-108](docs/decisions/RT_PDR_00_Decision_Log.md)):

  ```sh
  node tools/radar/generate.ts --check   # exit 1 se divergono
  node tools/radar/generate.ts           # riscrive gli otto SVG
  ```

  Zero dipendenze: Node 22 esegue TypeScript con type stripping, i test usano `node:test`. Gli SVG
  **non si editano a mano** — sono output, la correzione si fa sul catalogo o sulla rubrica.

DoD applicabile: compila Game+Editor · test mirati + regressione pertinente · determinismo/authority/privacy
preservati · TurnLog/reason code sufficienti · docs aggiornate · nessun warning/file generato/secret nuovo ·
verifica packaged quando richiesta dal checkpoint.

## Issue: il Tracking Impact Pass

Una Issue non è un elemento isolato. Quando ne **crei, spezzi o modifichi nella sostanza** una, il resto del
progetto ne registra gli effetti: milestone/epic, Feature Map, Scenario Map, test, Editor Map, asset,
content/data, Wiki/docs, ADR/Decision, UI/UX, debug/observability, dipendenze.

> **CREATE OR LINK, NEVER IGNORE.** Cerca prima un elemento esistente, collegalo, aggiornalo se lo scope
> cambia, crealo solo se manca davvero, e dichiara `N/A` quando non si applica.

Il blocco `## Tracking` è precompilato da [`.github/ISSUE_TEMPLATE/task.md`](.github/ISSUE_TEMPLATE/task.md):
`N/A` è valido, un campo cancellato no. Non inventare voci per riempire un campo — un duplicato sporca le
viste generate e si trova due volte.

Il pass si **riesegue prima di chiudere**: una Issue chiusa con tracking incoerenti è incompleta.

Regola completa, con le dodici categorie e cosa scrivere in ciascuna:
[`docs/technical/tooling/issue-tracking-completeness.md`](docs/technical/tooling/issue-tracking-completeness.md).

## Git

Repository: `DegrassiAaron/refactor-tactics-main`.

- Branch focalizzati: `feat/`, `fix/`, `refactor/`, `docs/`, `test/`.
- Conventional Commits.
- Controlla status/diff prima del commit.
- Niente commit, push, merge, force, delete remoti o operazioni distruttive senza richiesta esplicita.
- Non confondere “ho modificato i file” con “ho verificato build/PIE/packaged” (§*Test e Definition of Done*).

### Una sessione esecutiva = un worktree

Due sessioni nella stessa working directory si scrivono addosso: stesso file, stesso `git status`, branch
cambiato sotto i piedi dell'altra. Se te ne accorgi, **non «gestirlo con attenzione»**: dillo, e la task
va spostata in un worktree dedicato.

```powershell
git worktree add ..\rt-wt-621 -b feat/621-geometry-bake origin/main
```

Non creare né distruggere worktree senza richiesta esplicita: vale la regola non distruttiva qui sopra.

### File non assegnato = STOP

Il worktree isola la working directory, non il repository: due sessioni possono ancora scrivere lo stesso
file, e finora l'unica difesa era che se ne accorgesse il merge. Un lotto di sessioni parallele dichiara
prima chi scrive cosa in [`docs/roadmap/parallel-batch.yaml`](docs/roadmap/parallel-batch.yaml)
([D-139](docs/decisions/RT_PDR_00_Decision_Log.md)).

Prima di modificare un file, il path deve appartenere al `writable` della tua track. Altrimenti **ci si
ferma** e si registra una richiesta di riallocazione: non si fa «solo questa piccola fix». Vale per C++,
docs, scripts, Config, `.uasset`, `.umap`, test e output generati.

Tre categorie non si assegnano: `integration_only` si aggiorna **una volta** in integrazione;
`generated_only` **segue la propria sorgente** — chi possiede la sorgente rigenera quella vista, ed è
l'unico a poterlo fare; se due track alimentano la **stessa** vista non è un conflitto (`Tests/` lo è per
quasi tutte), ma nessuna delle due copie è autoritativa finché non si rigenera sull'albero unito; `preexisting` sono i branch già vivi, che non fanno parte del batch ma lo determinano. Il
write-set di un branch aperto si **misura**, mai si intuisce — dichiararlo a memoria ha già prodotto una
collisione al primo batch:

```powershell
git worktree list
gh pr list --state open
git diff --name-only origin/main...<branch>
```

### `D-nnn` non si sceglie a mano

```powershell
python scripts/rt_shared_id.py reserve D --reason "<issue/task>"
```

Stampa l'ID da usare, e **si usa esattamente quello**. È vietato dedurlo — *«l'ultimo è D-135, quindi
prendo D-136»* è la race, non la sua mitigazione: due sessioni che leggono lo stesso stato scelgono lo
stesso numero, e il progetto ne ha pagate **sedici** ([D-135](docs/decisions/RT_PDR_00_Decision_Log.md),
tutte registrate in fondo al Decision Log, più una su `E21`).

L'allocatore è atomico fra tutti i worktree di **questo clone** e considera già preso ogni ID che compaia
in un working tree, in un branch locale o in uno remoto — anche non committato, anche non mergiato. Non
copre altri cloni o altri PC: per quelli c'è `audit-refs`, che diagnostica invece di prevenire.

Prima della consegna, i due gate (il `fetch` è tuo, lo script non fa rete):

```powershell
python scripts/rt_shared_id.py check
git fetch --prune origin
python scripts/rt_shared_id.py audit-refs
```

Se `audit-refs` è rosso, **rinumera prima del merge**: `reserve` per un ID nuovo, rimandi corretti per
coppia `(file, riga)` — mai con una sostituzione globale — contenuto invariato. Se l'altra è già su
`main`, rinumeri la **seconda** e la registri nelle Note.

⚠️ Restano a mano gli altri contatori condivisi — numeri di **epic** (`Enn`) e ID di decisione aperta
(`XXX-n`): si verificano sul remote (`gh issue list --search "EPIC in:title"`) subito prima del merge,
perché il controllo *«qual è il primo numero libero»* scade mentre lavori. L'estensione dell'allocatore a
questi namespace è deliberatamente rimandata a dopo che `D-nnn` avrà dimostrato il meccanismo.

Meccanismo, recovery e cleanup: [`docs/technical/tooling/workflow-parallel-claude.md`](docs/technical/tooling/workflow-parallel-claude.md).

## Lingua

Rispondi e commenta in **italiano**; termini tecnici e identificatori di codice restano in inglese.
Il tutoring C++/UE è su richiesta, non il default.
