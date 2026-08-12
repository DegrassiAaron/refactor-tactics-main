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

Roster v0.1 corrente: **Flux · Riva · Bastion · Vektor**.
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

`docs/src/` contiene i sorgenti **non ancora consumati** (PRD di visione, dataset, media): **non è fonte
normativa per default**. *(Dal 2026-08-12 sono Markdown: in `docs/` non c'è più prosa in formato binario —
[D-009](docs/decisions/RT_PDR_00_Decision_Log.md).)* `docs/archive/` è storico — e dal 2026-08-08 include
[`docs/archive/src/`](docs/archive/src/README.md), dove finiscono i sorgenti **già recepiti** (design, handoff,
audit) con l'indice di chi li possiede oggi. Se cerchi la provenienza di una regola, è lì; se cerchi la regola,
è nell'owner.

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
- Thin slice predittivo v0.1: **`Vektor.InterceptShot`**.
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
- Naming e dipendenze contenuti: **`docs/technical/convenzioni-contenuti-ue.md`** è normativo.
- Terze parti/Paragon restano fuori da `/Game/RT` salvo pipeline esplicitamente documentata.
- Non modificare `.uasset`/`.umap` a mano e non spostarli da filesystem: usare Content Browser + Fix Up Redirectors.
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

DoD applicabile: compila Game+Editor · test mirati + regressione pertinente · determinismo/authority/privacy
preservati · TurnLog/reason code sufficienti · docs aggiornate · nessun warning/file generato/secret nuovo ·
verifica packaged quando richiesta dal checkpoint.

## Git

Repository: `DegrassiAaron/refactor-tactics-main`.

- Branch focalizzati: `feat/`, `fix/`, `refactor/`, `docs/`, `test/`.
- Conventional Commits.
- Controlla status/diff prima del commit.
- Niente commit, push, merge, force, delete remoti o operazioni distruttive senza richiesta esplicita.
- Non confondere “ho modificato i file” con “ho verificato build/PIE/packaged”.

## Lingua

Rispondi e commenta in **italiano**; termini tecnici e identificatori di codice restano in inglese.
Il tutoring C++/UE è su richiesta, non il default.
