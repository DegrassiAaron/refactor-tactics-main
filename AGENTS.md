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
7. Issue/task corrente, specifica di feature, cataloghi in `docs/balance/`, test e codice esistente.

> ⛔ **Il passo 7 era `feature-registry.yaml`, e diceva che «lo stato di una feature vive qui e in nessun
> altro posto». Quel file non esiste più** dal 2026-08-21
> ([D-181](docs/decisions/RT_PDR_00_Decision_Log.md)): il Feature Registry, le sue cinque `*.shortlist.md`
> e le due viste JSON sono usciti dal repository. ⚠️ **Non c'è un sostituto, e va detto**: lo stato di una
> feature non ha più un owner unico — si ricava dal codice, dai test e dalla roadmap scritta a mano, che
> sono tre fonti e non una. Il rischio che quel passo chiudeva è tornato aperto, per scelta.

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
- I binari Unreal sono **human-first**: l'autore davanti all'Editor è l'holder predefinito, e una
  sessione Claude li tocca solo su richiesta esplicita, attraverso Unreal. Due `.uasset` **non si
  fondono**: un conflitto binario è una delle due versioni da rifare a mano dentro l'Editor, quindi
  un binario si modifica da un lavoro solo per volta.
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
  di quanti ne esistono restando verde (`#486`). ⛔ **Il comando che lo verificava non esiste più**:
  `feature_registry.py suite --run-log` confrontava i test *dichiarati* con quelli *eseguiti* ed è uscito
  col Feature Registry il 2026-08-21 (**D-181**). Oggi il confronto si fa a mano leggendo il filtro nel log.
  Vale soprattutto prima di dichiarare una **verifica di mutazione**: se il test atteso non era in lista, il
  suo «non è caduto» non significa niente — e adesso nessuno script te lo dice.
- 🔴 **Una console variable in testa a `-ExecCmds` fa saltare l'intera coda di automation**, ed è il caso
  estremo della riga qui sopra: «N eseguiti su M dichiarati» con **N = 0**. Con
  `-ExecCmds="rt.Qualcosa 1; Automation RunTests <filtro>; Quit"` l'editor esegue la prima voce — la CVar
  compare nel log come `rt.Qualcosa = "1"` — e all'automation **non arriva**: nessuna riga
  `LogAutomationCommandLine`, nessun test registrato, e il file finisce a metà avvio somigliando a una run
  riuscita. Misurato il 2026-08-24 su due run consecutive, headless `-unattended -nullrhi`
  ([#1300](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1300)).
  ⚠️ **Come ci si accorge**: nel log devono esserci `Found <n> automation tests based on '<filtro>'` e, in
  fondo, `**** TEST COMPLETE. EXIT CODE: <n> ****`. Se manca la prima, la run non ha misurato niente.
  ✅ **La via d'uscita è `-dpcvars="nome=valore"`**, misurata il **2026-08-26** rigenerando il corpus
  golden con `rt.Test.RegenerateGolden`: imposta la CVar **senza toccare la coda**, quindi dentro
  `-ExecCmds` resta solo l'automation. ⚠️ Verifica sempre l'effetto atteso — `git status` sui file che
  dovevano cambiare — invece di fidarti dell'esito del test.
  ✅ In alternativa le impostazioni si passano dalla **riga di comando** e si leggono con
  `FParse::Value(FCommandLine::Get(), TEXT("RTQualcosa="), Out)`: così dentro `-ExecCmds` resta solo la coda
  di automation. ⚠️ Misurato con una CVar in **prima** posizione: che una voce non-CVar in testa faccia lo
  stesso non è stato provato.
- Le verifiche PIE/Editor non sono verdi finché qualcuno non le ha realmente eseguite.
- ⛔ **Il repository non ha più gate Python.** Restano **Node 22** in `tools/radar/` — rubrica dei
  rating, generatore SVG dei radar e allineatore degli alt sulla Wiki — e **tre** file Python
  versionati, nessuno dei quali è un gate: `tools/icons-downloader/paragon_skill_icons_downloader.py`
  è un **downloader**, e i due di `tools/decision-log/` sono un **generatore di vista** e il suo
  raccoglitore di dati. Non hanno `--check`, non escono `1` su una divergenza, e nessun DoD li nomina.
  *(Corretto il 2026-08-27: questa riga diceva «un solo file Python versionato», ed era vera fino a
  quando `tools/decision-log/` non è atterrato. Il difetto è quello che `CONTEXT_INDEX.md` §«Le due
  toolchain» ha già dichiarato per i gate: un elenco scritto a mano non si accorge di un file nuovo,
  e chi aggiunge il file non passa di qui.)* I controlli vivi sono **cinque**, più una suite: `node tools/radar/generate.ts --check` (gli SVG contro i cataloghi), `node tools/radar/wiki-alt.ts --wiki-root <clone> --check` (gli alt sulla Wiki, che il primo **non** copre — lo dichiara il suo stesso docstring), `node tools/radar/doc-links.ts --check` (i percorsi citati dai documenti), `node tools/radar/catalog-code.ts` (le stat base degli eroi fra catalogo e C++ — non ha `--check` perché non scrive mai: esce `1` e basta) e `node tools/radar/doc-tables.ts --check` (le righe di tabella che non hanno la larghezza delle sorelle), piu' la suite `node --test` di `tools/radar/` — si lancia **da dentro la cartella**, `node --test tools/radar/` fallisce con `MODULE_NOT_FOUND`.
  La cartella `scripts/` è stata **rimossa** il 2026-08-21 (**D-182**): con lei sono usciti i nove
  script Python e i loro test — i cinque gate documentali (link, nomi, simboli, tabelle, inventario),
  i due controlli sui dati di gioco (`check-capability-owners`, `check-equipment-defaults`) e i due
  generatori (`build-icon-assets`, `build-state-matrices-xlsx`).
  ⚠️ **Ciò che si perde va saputo**: nessuno verifica più che i dati di equipaggiamento combacino col
  C++, né che le **296 icone** di
  `docs/generated/icons/` corrispondano alla geometria che le ha prodotte — quella geometria era
  dichiarata dentro `build-icon-assets.py` ed è uscita con lui. È una scelta di fase: il progetto è in
  sviluppo, e il costo di mantenere i gate superava quello di non averli.
  ✅ **Link ed etichette sono tornati coperti il 2026-08-25** con `doc-links.ts`, in Node e dentro la
  toolchain che D-182 aveva già preservato: non riapre la decisione, ne ripara la conseguenza più
  costosa. Copre `docs/` più `AGENTS.md`, `CLAUDE.md` e `README.md`, ed è verde anche con
  `--with-archive`. Ciò che **non** copre è nel suo docstring — ancore, URL, percorsi in prosa, e gli
  altri Markdown della radice, che sono materiale importato — e va letto prima di fidarsene.
  ✅ **Anche le tabelle, di nuovo.** `doc-tables.ts` è uscito il 2026-08-26 con
  [D-192](docs/decisions/RT_PDR_00_Decision_Log.md) e **rientra lo stesso giorno** con [D-193](docs/decisions/RT_PDR_00_Decision_Log.md): la
  decisione ne aveva rimossi **due** — il controllo di larghezza e il tentativo di estenderlo alle righe
  staccate dalla loro tabella — mentre il costo misurato, quattro cicli di review senza convergere,
  riguardava solo il secondo. Il primo era verde, 157 righe, e all'ingresso aveva trovato **otto** righe
  rotte davvero.
  ⛔ **Resta scoperto** il caso che ha aperto la vicenda: una riga di tabella **staccata** dalla sua
  tabella da una riga vuota è invisibile a questo controllo **per costruzione** — confronta le righe di un
  blocco fra loro, e un blocco di una riga non ha sorelle. Sul corpus di oggi sono **18** blocchi che il
  gate **dichiara** di non confrontare, stampandoli sotto la riga di copertura: `tabelle confrontate:
  1521`, non 1539. ⚠️ **Altri tre limiti e un falso positivo noto sono nel suo docstring** — una riga
  indentata zittisce l'intera tabella, un difetto maggioritario fa segnalare le righe sane, e la pipe
  finale che GFM rende facoltativa produce un rosso su markdown valido — e vanno letti prima di fidarsene,
  come per `doc-links.ts`.
  Ciò che resta si esegue **a mano**, ed è una scelta:
  **`.github/workflows/` non esiste, e la sua assenza è deliberata.** Non introdurre CI, package manager o
  build step senza chiedere: `tools/radar/` ha **zero dipendenze** apposta.
  *(Corretto il 2026-08-16: la riga diceva «`.github/` non esiste», e dal 2026-08-16 è falsa —
  `.github/ISSUE_TEMPLATE/` esiste e precompilava il blocco `## Tracking` fino al 2026-08-21, quando **D-181** l'ha tolto. La decisione proteggeva la **CI**,
  non la cartella, e l'oracolo che la verificava — `git ls-tree … -- .github/` — è stato ristretto a
  `workflows/` nello stesso commit. Un template non esegue niente; un workflow sì. Precedente del
  2026-08-12: diceva «è vuota», che induceva a cercare una cartella inesistente.)*
- ⚠️ **`tools/radar/` non è solo documentazione.** I rating si calcolano dai cataloghi di bilanciamento
  ([D-106](docs/decisions/RT_PDR_00_Decision_Log.md)), quindi cambiare `Salute`, un danno o un cooldown
  rende rossi gli SVG versionati finché non li rigeneri **nello stesso commit**
  ([D-108](docs/decisions/RT_PDR_00_Decision_Log.md)):

  ```sh
  node tools/radar/generate.ts --check   # exit 1 se divergono
  node tools/radar/generate.ts           # riscrive gli otto SVG
  node tools/radar/doc-links.ts --check  # exit 1 se un percorso citato non risolve
  node tools/radar/catalog-code.ts       # exit 1 se catalogo e C++ divergono, o se la copertura cala
  node tools/radar/doc-tables.ts --check # exit 1 se una riga di tabella ha piu' o meno celle delle sorelle
  cd tools/radar && node --test          # la suite: da dentro la cartella, non dalla radice
  ```

  Zero dipendenze: Node 22 esegue TypeScript con type stripping, i test usano `node:test`. Gli SVG
  **non si editano a mano** — sono output, la correzione si fa sul catalogo o sulla rubrica.

DoD applicabile: compila Game+Editor · test mirati + regressione pertinente · determinismo/authority/privacy
preservati · TurnLog/reason code sufficienti · docs aggiornate · nessun warning/file generato/secret nuovo ·
verifica packaged quando richiesta dal checkpoint.

## Git

Repository: `DegrassiAaron/refactor-tactics-main`.

- Branch focalizzati: `feat/`, `fix/`, `refactor/`, `docs/`, `test/`.
- Conventional Commits.
- Controlla status/diff prima del commit.
- Niente commit, push, merge, force, delete remoti o operazioni distruttive senza richiesta esplicita.
- Non confondere “ho modificato i file” con “ho verificato build/PIE/packaged” (§*Test e Definition of Done*).

### Più sessioni, e la misura che non mente

Lo sviluppo è **parallelo di fatto**, e va trattato come tale
([D-222](docs/decisions/RT_PDR_00_Decision_Log.md), che supera la clausola operativa di
[D-178](docs/decisions/RT_PDR_00_Decision_Log.md)). ⛔ Questa sezione diceva *«lo sviluppo è
sequenziale: una sessione, una working directory, un branch alla volta»*, e **descriveva un regime che
non è quello praticato**: il 2026-08-27 sono stati misurati **101** checkout di `HEAD` in 24 ore,
**6 sessioni** distinte a committare e **4 nella stessa finestra di 6 minuti**, tutte sullo stesso
worktree.

Più sessioni condividono quindi disco, `HEAD` e binario, e continuano a scriversi addosso. Ciò che
cambia è **cosa si protegge**: non la working directory, che nessuno può riservarsi, ma la **misura**.

> Una suite vale solo se `HEAD`, l'albero di lavoro, il binario e i processi del motore sono gli stessi
> all'inizio e alla fine. Altrimenti non è né rossa né verde: è **NON VALIDA**.

Il difetto che questo chiude non è il parallelismo: è il **silenzio**. Nessuna di queste collisioni
produce un errore — producono verde che misura un'altra cosa. Una suite `1233/1233, 0 fail` che aveva
letto un file cambiato a run iniziata; due suite morte a **641/1175** e **662/1191** con `Fail = 0`.

**Per la suite si usa [`scripts/rt-suite.ps1`](scripts/rt-suite.ps1)**, che quei controlli li fa sempre
invece di ricordarseli. Da **PowerShell**: Git Bash traduce gli argomenti che iniziano con `/` e
l'harness non parte nemmeno.

⚠️ **I worktree non sono la via d'uscita**: il mutex Live Coding è globale sull'eseguibile del motore,
quindi due run di automation si uccidono a vicenda anche da checkout diversi. Isolare il disco
lascerebbe scoperta proprio la collisione più silenziosa.

⚠️ **Il merge resta scoperto**: il 2026-08-27 due PR sono state mergiate **prima che il proprio gate
finisse**, entrambe da un'altra sessione. Uno script locale non lo intercetta. Prima di mergiare,
verifica che il gate sia girato sul commit che stai mergiando.

### ID condivisi: `D-nnn`, `Enn`, `XXX-n`

Tre contatori vivono in documenti diversi e nessuno li assegna. Il numero si legge dall'ultimo
assegnato e si **riverifica subito prima del merge**, perché la risposta a *«qual è il primo numero
libero»* scade mentre lavori:

```powershell
git fetch --prune origin
git grep -oh "D-[0-9]\+" origin/main -- docs/decisions/RT_PDR_00_Decision_Log.md | sort -V | tail -1
gh pr list --state open
gh issue list --search "EPIC in:title"
```

Un branch aperto e un working tree non committato sono rivendicazioni quanto un merge. Se una PR in
volo dichiara lo stesso ID con una **tesi diversa**, rinumera la seconda: rimandi corretti per coppia
`(file, riga)` — mai con una sostituzione globale — contenuto invariato. Se l'altra è già su `main`,
la seconda sei tu, e la registri nelle Note.

⚠️ **Limite noto, non aggirato — e più largo di quanto questa riga dicesse.** È un controllo a vista, e
il progetto ha già pagato **sedici** collisioni con lo stesso metodo prima di automatizzarlo. ⛔ Qui
seguiva *«con una sola sessione per volta la finestra di race si chiude quasi tutta»*: quella premessa è
**falsa**, misurata il 2026-08-27 ([D-222](docs/decisions/RT_PDR_00_Decision_Log.md)) — 101 checkout in
24 ore e 6 sessioni a committare, che è lo stesso regime in cui quelle sedici collisioni erano avvenute.
La finestra resta aperta quanto le PR non mergiate, e il `fetch` prima del merge non è facoltativo.

## Lingua

Rispondi e commenta in **italiano**; termini tecnici e identificatori di codice restano in inglese.
Il tutoring C++/UE è su richiesta, non il default.
