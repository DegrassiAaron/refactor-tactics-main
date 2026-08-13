> 📦 `HISTORICAL` · **Sorgente archiviato il 2026-08-13** · **Revisionato, recepito in parte.**
>
> Il testo originale **non è stato riscritto**: quanto segue è l'esito della revisione, non una correzione
> del sorgente. Referto completo:
> [`../../../roadmap/plans/facing-visualdocs-triage-2026-08-13.md`](../../../roadmap/plans/facing-visualdocs-triage-2026-08-13.md).
>
> | Esito | Sezioni |
> |---|---|
> | ✅ **Recepito** — contributo nuovo | §3 (`FAC-11` → **D-126**, ma nel verso opposto a quello implicito: vedi sotto) · §6/§8/§25 (import dei sette asset e mappa immagini→pagine) · §11 (riconciliazione issue) · §12 (**1** scenario su 5) |
> | **Già canone** — nessuna azione | §5 (principi del facing) · §17 (privacy) · §18 (determinismo/TurnLog) · §19 (gerarchia delle fonti) |
> | ⚠️ **Recepito con esito diverso** | §3.1/§3.2: i sei lati diventano la primitiva **semantica**, ma `HexCone` **non** viene sostituito nei consumatori. Misurato: il cono è **strettamente contenuto** nell'insieme dei tre lati (**50** celle di divergenza su raggio `1..10`, **zero** nel verso opposto), quindi la sostituzione sarebbe un **buff difensivo**, non una rinomina |
> | 🔴 **Difetto del pacchetto** | I file `F4_*` e `F6_*` contenevano il diagramma **l'uno dell'altro**. I sette SHA-256 e i byte corrispondono al manifest §27: l'audit per hash prescritto da §8 dà **verde**, perché a mentire erano i **nomi**. Corretti in entrambe le sedi |
> | 🔴 **Difetto del pacchetto** | §7 avverte su `FAC-12` per F1/F2 — verificato aprendo le immagini: **non contengono** il costo in MP, sono allineate. Ma **F3**, dichiarata «canonica dopo FAC-11», disegna nel pannello 1 la policy della direzione d'impatto, che è **`FAC-13`, aperta**: il pacchetto non lo sapeva |
> | ✂️ **Filtrato** | 5 scenari proposti → **1** creato (gli altri 4 chiederebbero all'harness capacità che il gioco non ha) · issue nuove: **1** (#726), non una epic |
> | ⛔ **Non fatto per divieto rispettato** | Nessuna seconda epic Facing (E16 resta chiusa) · nessuna feature spezzata (**0** gate cambiati) · `Guard`/`Brace` invariati · `FAC-12`/`FAC-13`/`FAC-14` restano aperte |

# RefactorTactics — APPLY PACK
## Facing: documentazione, Wiki, roadmap, issue/epic, scenario map e asset visuali
**Data handoff:** 2026-08-13  
**Destinatario:** Claude Code / Claude CLI  
**Repository:** `DegrassiAaron/refactor-tactics-main`

> Questo file è un **handoff operativo di consolidamento**, non una nuova sorgente di verità.
> Prima di modificare qualunque file devi misurare lo stato reale di `main`, leggere le decisioni correnti e
> riconciliare il contenuto qui sotto con il repository. Le decisioni esplicite più recenti dell'autore indicate
> in questo handoff vanno formalizzate nel sistema decisionale del repository; tutto il resto va trattato
> secondo la gerarchia documentale già in vigore.

---

# 0. Obiettivo

Consolidare il lavoro sul **Facing** e i sette diagrammi visuali appena prodotti in modo che il repository e
la Wiki abbiano una sola storia coerente.

Devi:

1. verificare lo stato corrente di codice, roadmap, issue, epic, feature registry, scenario map e Wiki;
2. formalizzare la decisione più recente sui **sei lati del Facing**;
3. aggiornare ADR / Decision Log / open decisions senza duplicare fonti;
4. correggere documenti e issue diventati stale;
5. aggiornare roadmap e mappe di avanzamento;
6. verificare gli scenari realmente esistenti e creare solo quelli realmente mancanti;
7. aggiornare la Wiki **nel clone Wiki**, non creando copie delle pagine dentro `docs/wiki/`;
8. importare e usare i **sette PNG separati** nelle pagine appropriate;
9. mantenere privacy, determinismo, TurnLog e server authority;
10. produrre un report finale misurato con file/issue/scenari realmente toccati.

---

# 1. Prima regola: audit prima dell'apply

Prima di modificare:

```bash
git status
git branch --show-current
git rev-parse HEAD
git pull --ff-only
```

Poi leggere almeno:

```text
CLAUDE.md
AGENTS.md
README.md

docs/decisions/RT_PDR_00_Decision_Log.md
docs/decisions/adr-0005-orientamento.md
docs/decisions/adr-0008-rotazione-e-policy-di-facing.md
docs/OPEN_DECISIONS.md
docs/DOC_CONFLICT_MATRIX.md

docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/roadmap.shortlist.md
docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.md
docs/roadmap/scenariomap.shortlist.md
docs/technical/scenario-map.md
docs/roadmap/editormap.shortlist.md
docs/technical/test-manuali-pie.md

docs/roadmap/plans/facing-consolidation-triage-2026-08-10.md
docs/archive/src/handoff/2026-08-10-facing-consolidation.md

docs/wiki/README.md
```

Poi misurare:

```bash
git grep -n -i "Facing"
git grep -n "FAC-"
git grep -n "Spec.Facing"
git grep -n "Overwatch" Scenarios docs/roadmap docs/gameplay Source/RefactorTactics/Tests
```

E interrogare GitHub per issue/epic correnti.

**Non usare numeri o stati riportati in questo handoff come sostituto dell'audit.**

---

# 2. Stato misurato quando questo handoff è stato preparato

Questi fatti servono come controllo anti-regressione, ma vanno rimisurati all'esecuzione.

## 2.1 Facing core

- E16 / Facing core esiste già ed è stata chiusa.
- `Facing` è già stato trattato come stato logico/autorevole.
- Esistono già ADR dedicati (`ADR-0005`, `ADR-0008`).
- Esistono già utility/policy di rotazione dichiarata.
- Non creare una seconda epic `Tactical Facing & Directional Interaction`.
- Non creare una seconda feature family spezzando `RT-FEAT-MAP-FACING` in molte feature duplicate.

## 2.2 Issue ancora rilevanti

Verificate aperte durante l'audit del 2026-08-13:

```text
#339  [DESIGN] Facing: decisioni aperte / FAC-*
#291  rotazione dichiarata: runtime cablato, manca input giocatore/bot e scenario end-to-end
#172  CP 11.5 Ghost Timeline
#152  E14 Overwatch e reazioni interattive
#25   E11 HUD, log e debug
#589  privacy strutturale / packaged canary
```

### Attenzione a #172

Il corpo di `#172` contiene ancora una frase stale equivalente a:

> il facing è derivato dalla presentazione e non è un dato autorevole

Questa frase non è più accettabile rispetto al sistema corrente.

La preview può **derivare una previsione** dal piano, ma il Facing competitivo appartiene allo stato logico e
alla simulazione autorevole. Il renderer/view model non deve diventare una seconda autorità.

**Aggiornare #172** in modo che il ghost mostri:

- Facing corrente/autorevole quando noto;
- Facing previsto dal proprio/team intent sanitizzato;
- legal facings / planned facing quando supportati;
- certainty `Confermato / Previsto / Incerto`;
- nessun planned Facing avversario.

## 2.3 E14 / Overwatch

Durante l'audit corrente:

- `#164` CP 14.4 — Overwatch armato e trigger a micro-step — risulta **chiuso**;
- il percorso attivo di E14 continua da `#165`, poi `#166`, con `#314/#319` come estensioni.

Qualunque tabella/immagine che mostri `#164` come ancora planned/open è una **snapshot stale** e non va
spacciata per live status.

---

# 3. Decisione esplicita più recente da formalizzare — FAC-11

Questa è la decisione nuova che deve superare la vecchia ambiguità.

## 3.1 Primitive del Facing

Il Facing fondamentale dell'unità coincide con i **sei lati dell'esagono**.

Esistono sei direzioni discrete e cicliche:

```text
0 = Front
1 = FrontRight
2 = RearRight
3 = Rear
4 = RearLeft
5 = FrontLeft
```

Il `Front` è il lato verso cui l'unità è orientata.
Il `Rear` è il lato opposto.
Le altre quattro direzioni restano **distinte**, non vengono collassate in un generico `Side/Flank`.

Il modello competitivo deve continuare a usare una direzione intera/discreta `0..5`, non un `FRotator`
come autorità.

## 3.2 Conseguenza semantica

La primitiva è:

```text
IncomingDirection relative to TargetFacing -> una delle 6 RelativeDirection
```

Una specifica abilità, difesa o reaction può raggruppare più direzioni.

Esempi **illustrativi**, non valori universali:

```text
Narrow Parry         = { Front }
Wide Shield          = { FrontLeft, Front, FrontRight }
Rear-only ability    = { Rear }
Flank-like ability   = { RearLeft, RearRight }
```

Questi insiemi appartengono al **consumatore**.

Non esiste più, come semantica fondamentale del Facing, una divisione globale obbligatoria:

```text
Front Arc / Flank / Rear
```

`HexCone` può continuare a esistere come utility/shape spaziale dove serve, ma **non deve essere confuso con
la primitiva semantica del Facing**.

## 3.3 Cosa fare con ADR-0008 e FAC-11

Il triage del 2026-08-10 registrava `FAC-11` proprio perché la proposta dei sei lati individuali era in conflitto
con la frase di ADR-0008 secondo cui un unico `HexCone` governava difesa/percezione/reazioni.

Ora l'autore ha preso una decisione esplicita: **i sei lati sono la primitiva**.

Quindi:

1. non lasciare `FAC-11` come domanda aperta;
2. aggiornare/formalizzare la decisione secondo le convenzioni del repository:
   - emendamento di ADR-0008 oppure nuova decisione nel Decision Log, scegliendo la forma già usata dal repo;
3. aggiornare `OPEN_DECISIONS.md`;
4. aggiornare `DOC_CONFLICT_MATRIX.md`;
5. aggiornare `#339`;
6. cercare consumatori di `IsInFrontalArc` / `HexCone` e classificare:
   - consumer che può restare un cone;
   - consumer che invece deve interrogare la nuova relazione a sei direzioni;
7. **non cambiare automaticamente il balance di Guard/Brace** solo per effetto della nuova primitiva.

---

# 4. Cose NON decise da questo apply

Non trasformare in canone proposte che restano aperte.

## FAC-12 — Pivot a costo MP

Il diagramma/proposta precedente ha esplorato:

```text
1 Pivot step = 1 Movement Point
```

ma ADR-0008 usa oggi un modello a **tetto di step**, non necessariamente a prezzo MP.

Questa decisione resta da verificare/risolvere come `FAC-12`.

Non cambiare il runtime soltanto perché un'immagine mostra "costo MP".

## FAC-3 — Brace direzionale

Non è automaticamente risolta dalla primitiva a sei lati.

Lo scenario corrente:

```text
Spec.Facing.BraceHoldsFromBehind
```

pinna un comportamento preciso.

Se si vuole cambiare Brace, deve passare dalla decisione/balance owner corrente e dallo scenario esistente:
**non aggiungere un nuovo scenario contraddittorio lasciando quello vecchio verde**.

## FAC-5..FAC-9, FAC-13, FAC-14

Restano decisioni/proposte finché non risultano già formalizzate da commit successivi.

In particolare:

- Reaction Pivot;
- Interact + Facing;
- status che limitano rotazione;
- terreno che limita rotazione;
- orientation-aware pathfinding;
- direzione di impatto non puntuale;
- forced rotation come effetto di controllo.

Auditare prima di cambiare.

---

# 5. Principi Facing che restano validi

Consolidare senza duplicare:

- Facing persistente nello stato logico;
- snapshot/TurnLog/replay devono poter spiegare il Facing usato;
- animazione e `ActorRotation` non decidono l'esito competitivo;
- movimento riuscito può aggiornare il Facing dalla transizione;
- un passo bloccato non deve regalare una rotazione implicita;
- nessuna correzione gratuita durante Resolution;
- nessun automatic snap-back dopo un'azione;
- attack/reaction/displacement usano policy esplicite;
- forced movement e forced facing sono concetti separati;
- cover ambientale e Facing restano sistemi separati;
- pathfinding, LOS, targeting, trajectory e Facing restano servizi/concetti separati;
- nessun bonus universale `rear = +X% damage`;
- un'abilità può avere bonus/requirement direzionali specifici;
- planned Facing/Pivot è team-only;
- il nemico vede solo Facing corrente osservabile / last-known secondo Team Knowledge;
- niente leak del future Facing;
- i reason code e il TurnLog devono permettere explainability.

---

# 6. Asset visuali da importare

Il pacchetto contiene sette PNG separati.

Copia gli asset nel repository in una cartella coerente con `docs/wiki/README.md`.

Suggerimento, salvo convenzioni più aggiornate trovate nell'audit:

```text
docs/wiki/RefactorTactics_Facing_Flows_v0.1/
```

Non creare pagine Wiki in `docs/wiki/`: **D-076 dice che lì restano solo asset**.

I file sono:

| ID | File | Uso principale |
|---|---|---|
| F1 | `F1_Lifecycle_del_Facing_nel_Turno.png` | Overview Facing / lifecycle |
| F2 | `F2_Move_Micro_step_e_Pivot.png` | Movement + micro-step + Pivot |
| F3 | `F3_Attacco_e_Direzione_Relativa.png` | Pagina Facing: sei direzioni relative |
| F4 | `F4_Overwatch_Reaction_Facing.png` | Overwatch / Reaction |
| F5 | `F5_Forced_Movement_Facing_Control.png` | Displacement / Facing control |
| F6 | `F6_Privacy_Team_only_UI_del_Facing.png` | Privacy / team-only / UI |
| F7 | `F7_Scenario_Coverage_Map_Facing.png` | Coverage / avanzamento scenari |

---

# 7. Audit obbligatorio delle immagini prima della pubblicazione

Le immagini sono **visual design artifacts**, non autorità.

Leggile e confrontale col canone prima di usarle.

## F1 — Lifecycle

Usare nella pagina Facing overview.

⚠️ Se contiene wording che tratta il Pivot come **costo MP**, non presentarlo come regola canonica finché
`FAC-12` è aperta.

Soluzioni ammesse:

1. caption esplicita `Diagramma concettuale — costo Pivot soggetto a FAC-12`;
2. rigenerare/correggere l'asset prima di renderlo canonico;
3. usare la parte lifecycle senza citare il prezzo.

Non copiare un dato aperto dentro un ADR per "allinearlo all'immagine".

## F2 — Move, micro-step e Pivot

Usare su Facing/Movement.

Questa è la tavola migliore per spiegare:

```text
successful step -> update Facing
blocked step -> no implicit Facing change
Decision Boundary -> consumers read current Facing
final declared rotation -> after the legal boundary
```

Verificare la semantica corrente della rotazione dichiarata prima di descrivere costo/tetto.

## F3 — Attacco e direzione relativa

Questa è la tavola principale della nuova decisione dei **sei lati**.

Usarla nella Wiki `facing-e-direzionalita`.

⚠️ `Shield Front Arc = {5,0,1}` e `Backstab = {3}` sono **esempi illustrativi**:
non trasformarli in regole universali.

La parte canonica è:

```text
0 Front
1 FrontRight
2 RearRight
3 Rear
4 RearLeft
5 FrontLeft
```

## F4 — Overwatch / Reaction + Facing

Usare nella pagina Overwatch/reazioni.

Allineare la caption allo stato corrente:

- CP 14.4 / `#164` è già chiusa se l'audit lo conferma;
- il prossimo lavoro è la decision window / first consumer (`#165`);
- detection e LOS restano controlli separati;
- nessun planned Facing avversario entra nel client.

## F5 — Forced Movement & Facing Control

Usare soltanto come:

```text
PROPOSED / design direction
```

finché `FAC-14` non è formalizzata.

La separazione:

```text
where target goes
vs
what happens to Facing
```

è utile e coerente come modello.
L'esistenza di `RotateSteps`, `FaceSource`, ecc. come catalogo runtime va verificata.

## F6 — Privacy / Team-only / UI

Questa è adatta a pagina Facing + privacy/planning.

Deve restare coerente con:

- canonical intent server-only;
- team preview sanitizzata;
- public current state;
- last-known enemy Facing;
- zero future planned Facing avversario.

Collegare ai test anti-leak esistenti, incluso quello Facing, senza crearne duplicati.

## F7 — Scenario Coverage Map

⚠️ Questa immagine è **una snapshot datata**, non una dashboard live.

È già noto che può contenere almeno uno stato stale (`#164`).

Quindi:

- non usarla come unica fonte di avanzamento;
- inserirla in una pagina `stato / coverage` con caption:
  `Snapshot visuale generata il 2026-08-13 — la tabella live sotto è autorevole`;
- mantenere sotto di essa una tabella generata/misurata da scenario registry + issue correnti;
- se il clone Wiki usa una pagina di stato live, preferire lì la tabella live;
- rigenerare F7 in un secondo lavoro quando gli stati sono consolidati.

**Non falsificare il registry per farlo coincidere con l'immagine.**

---

# 8. Wiki — percorso corretto

`docs/wiki/README.md` stabilisce:

> le pagine non vivono più in `docs/wiki/`; la fonte è il clone pubblicato `refactor-tactics-main.wiki`.

Quindi:

1. trovare/aggiornare il clone Wiki;
2. modificare la pagina già esistente:
   ```text
   facing-e-direzionalita
   ```
   non crearne una seconda;
3. cercare pagine correlate:
   - movimento;
   - Overwatch / reazioni;
   - Brace / Guard;
   - planning / ghost;
   - privacy / Team Knowledge;
   - TurnLog / determinismo;
   - cover;
4. inserire i diagrammi nelle pagine appropriate;
5. aggiornare cross-link;
6. eseguire, se applicabile:

```bash
python scripts/feature_registry.py deploy --wiki-root <clone> --write
```

per aggiornare i blocchi di stato.

## Asset Wiki

`docs/wiki/` resta una asset library.

Prima di aggiungere immagini:

- confrontare hash contro asset già presenti;
- non dedurre duplicati dal nome;
- mantenere nomi stabili e leggibili;
- verificare come il clone Wiki referenzia le immagini oggi;
- seguire il pattern reale del clone invece di inventare una sintassi.

---

# 9. Pagina Wiki Facing — contenuto minimo aggiornato

La pagina `facing-e-direzionalita` deve spiegare almeno:

1. Facing = una delle sei direzioni dell'hex;
2. diagramma F3;
3. tabella:
   `Front / FrontRight / RearRight / Rear / RearLeft / FrontLeft`;
4. Facing persistente e autorevole;
5. come il movimento può aggiornare il Facing;
6. differenza fra movement-derived Facing e declared rotation;
7. Decision Boundary / micro-step con F2;
8. attack/reaction consumer;
9. ability-defined direction sets;
10. nessun universal backstab multiplier;
11. cover ambientale ≠ personal directional defense;
12. privacy:
    current observed vs future team-only;
13. TurnLog/replay;
14. link a Overwatch e planning Ghost;
15. sezione `Decisioni aperte` solo per FAC ancora veramente aperte.

La pagina non deve raccontare `HexCone` come sinonimo della primitiva del Facing dopo la formalizzazione di
FAC-11.

---

# 10. Documentazione repository da consolidare

Cercare prima di creare.

Aggiornare dove necessario:

```text
docs/decisions/adr-0005-orientamento.md
docs/decisions/adr-0008-rotazione-e-policy-di-facing.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/OPEN_DECISIONS.md
docs/DOC_CONFLICT_MATRIX.md

docs/gameplay/*
docs/technical/*
docs/roadmap/roadmap-v0.1.md
docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.md
docs/technical/scenario-map.md
docs/roadmap/scenariomap.shortlist.md
docs/roadmap/editormap.shortlist.md
docs/technical/test-manuali-pie.md
```

Non aggiornare file generated a mano se esiste uno script canonico per rigenerarli.

`feature-registry.yaml` è la sorgente: se `feature-registry.md/json` sono generated, rigenerarli con lo script.

---

# 11. Issue / Epic reconciliation

## E16

Non riaprire E16 e non duplicarla.

La nuova decisione FAC-11 può produrre:

- modifica documentale/ADR;
- delta su consumer;
- issue nuova **solo se esiste lavoro runtime realmente non posseduto**.

Se il cambio ai sei lati richiede un refactor consumer importante, collegarlo a E16 come legacy/related work,
ma non falsare lo stato storico della epic già chiusa.

## #339

Aggiornare.

Il corpo è già diventato incoerente nel tempo fra titolo, FAC elencate e vecchio testo.

Dopo questo apply:

- `FAC-11` deve risultare **DECIDED/CLOSED** con link alla decisione;
- `FAC-12/13/14` mantengono il loro stato reale;
- FAC già chiuse in commit successivi vanno rimosse dall'elenco open;
- correggere il conteggio nel titolo;
- non tenere frasi tipo "non oltre apertura E16" se E16 è già chiusa: datarle/archiviarle o riscriverle.

## #291

Non chiuderla solo perché esiste il resolver.

Restano da verificare:

- input giocatore;
- feedback legal facings;
- bot producer;
- scenario intent `facing`;
- scenario end-to-end.

Collegare F1/F2/F6 come riferimenti visuali se utile.

## #172

Aggiornare il contratto del ghost:

```text
NO: renderer inventa Facing autorevole
YES: ghost/viewmodel mostra il Facing previsto derivato dalle regole e dagli intenti autorizzati
YES: current logical Facing rimane autorità
```

Inserire F1/F2/F6 dove aiuta.

## #152 / E14

Aggiornare solo se manca il collegamento Facing.

Non riaprire `#164` se chiusa.

F4 deve supportare `#165/#166`, non riscrivere la storia dei CP chiusi.

## Nuove issue

Creare una nuova issue solo se:

1. il lavoro non è posseduto da una issue corrente;
2. il risultato atteso è deciso;
3. lo scenario/test ha un owner;
4. non duplica una feature/CP esistente.

---

# 12. Scenario audit — non fidarsi della vecchia lista

Durante l'audit del 2026-08-13 risultavano presenti almeno:

```text
Scenarios/Spec/Facing/DerivesFromMove.json
Scenarios/Spec/Facing/DashReorients.json
Scenarios/Spec/Facing/TargetingReorients.json
Scenarios/Spec/Facing/BraceHoldsFromBehind.json
Scenarios/Spec/Facing/FrontAttackKeepsGuard.json
Scenarios/Spec/Facing/BackAttackIgnoresGuard.json
Scenarios/Spec/Facing/RearHitOnCoverIsTraced.json
Scenarios/Spec/Facing/FrontHitOnCoverIsSilent.json
```

Rimisurare prima di aggiungere.

## Gap da verificare

Cercare scenari equivalenti, non solo nomi identici.

### A. Blocked step keeps Facing

Intento:

```text
tentativo di transizione in direzione diversa
-> blocco
-> unità resta nella cella
-> Facing non cambia per il solo tentativo
```

Nome suggerito se manca davvero:

```text
Spec.Facing.BlockedStepKeepsFacing
```

### B. Curved path uses last completed step

Serve un percorso che **cambia direzione**.

Il test deve dimostrare il Facing al boundary, non soltanto il Facing finale.

Nome suggerito:

```text
Spec.Facing.TurningPathUsesLastCompletedStep
```

Se l'harness non sa asserire uno stato intermedio:

- estendere l'assertion model minimale;
- oppure usare un consumer reale al boundary;
- non duplicare il resolver dentro lo scenario.

### C. Six relative sides

Dopo la chiusura di FAC-11, aggiungere una specifica eseguibile che dimostri la relazione a 6 lati.

Nome suggerito:

```text
Spec.Facing.SixRelativeSides
```

Preferire assertion sulla relazione/TurnLog rispetto a sei danni artificialmente diversi.

### D. Overwatch follows Facing

Verificare cosa #164 ha già aggiunto.

Se manca una coppia positiva/negativa:

```text
Spec.Overwatch.TargetInsideFacingCoverageTriggers
Spec.Overwatch.TargetOutsideFacingCoverageDoesNotTrigger
```

oppure il naming equivalente già scelto dal repository.

Mantenere identici:

- range;
- LOS;
- detection;
- readiness;

e cambiare solo la relazione direzionale.

### E. Ghost / preview Facing

Scenario visuale collegato a `#172/#291`:

```text
Visual.Planning.FacingGhost
```

Solo quando l'input e il view model possono davvero produrre il dato.
L'harness non deve diventare più capace del gioco.

---

# 13. Scenario Map / progress maps

Dopo l'audit:

1. aggiornare `docs/technical/scenario-map.md`;
2. rigenerare/aggiornare `docs/roadmap/scenariomap.shortlist.md`;
3. aggiornare il gate scenario di `RT-FEAT-MAP-FACING`;
4. mappare ogni scenario a:
   - feature;
   - issue;
   - milestone;
   - stato;
   - classe A/B/C;
5. inserire F7 come **visual snapshot datata**, non come database.

Stati ammessi devono seguire il vocabolario già usato dal repository.

Non introdurre una seconda tassonomia `GREEN/PLANNED/...` se il registry usa nomi diversi:
la legenda dell'immagine può restare visuale, la sorgente live usa il vocabolario canonico.

---

# 14. Feature Registry

`RT-FEAT-MAP-FACING` esiste già.

Non spezzarlo in 16 feature.

Aggiornare soltanto i gate realmente cambiati:

```text
runtime
automation
scenario
ui_wiki
privacy
log_debug
replay_representable
packaged
...
```

seguendo lo schema reale.

Se una parte è `na`, spiegare perché; non usarlo come sinonimo di "non fatto".

Poi eseguire i check/generator previsti da:

```bash
python scripts/feature_registry.py ...
```

Leggere `--help` prima di assumere i subcommand.

---

# 15. Roadmap

Aggiornare la roadmap **per delta reale**, non riscrivere E16.

Il percorso operativo da riflettere è:

```text
Decisione FAC-11 formalizzata
    ->
consumer / scenario six-side relation
    ->
#291 input rotazione dichiarata
    ->
#172 Ghost Timeline / Facing preview
    ->
E14 #165 / #166 Facing-aware reaction UI
    ->
QA / packaged / privacy gates
```

Se l'audit mostra che un punto è già chiuso, saltarlo.

Non riportare `#164` come prossimo lavoro se è già chiusa.

---

# 16. Editor Map

Il Facing richiede Editor work solo dove è veramente visuale.

Possibili voci da associare, non necessariamente nuove:

- visual forward axis della skeletal mesh;
- facing marker sul lato hex;
- legal-facing selector;
- final facing Ghost;
- directional overlay per reaction/defense;
- debug Facing;
- visual scenario `FacingGhost`.

Non mettere utility C++ pure o test headless dentro l'Editor Map.

---

# 17. Privacy

Planned Facing è un intento.

Quindi:

```text
server CanonicalIntentStore = completo
team preview = sanitizzata
enemy = MAI planned Facing/Pivot
```

Il Facing corrente risolto può diventare pubblico/observed secondo Team Knowledge.

Quando una unità sparisce da LOS:

- niente aggiornamento segreto;
- eventuale last-known Facing resta informazione storica.

Verificare i test esistenti prima di crearne altri:

```text
RefactorTactics.Facing.IntentIsTeamFiltered
```

e i canary generali.

F6 deve essere collegata a queste regole.

---

# 18. Determinismo / TurnLog

Facing deve rimanere rappresentabile nel log/replay.

Verificare:

- `FacingChanged` / equivalente corrente;
- reason/outcome correnti;
- movement-derived Facing;
- declared rotation;
- target/action reorientation;
- directional cover bypass già tracciato;
- permutation invariance.

Non cambiare il formato TurnLog senza misurare:

- versione;
- hash;
- golden corpus;
- reader compatibility.

F1/F2/F3 sono documentazione, non una ragione per cambiare formato.

---

# 19. PDR e file storici

I PDR v0.1 caricati possono essere usati come background per:

- determinismo;
- privacy;
- UI certainty;
- separazione path/LOS/targeting;
- modding/data validation.

Ma il repository corrente contiene ADR, Decision Log, roadmap e feature registry più recenti.

Regola:

```text
current explicit project decisions
> current ADR/Decision Log
> current canonical specs
> roadmap/registry
> old PDR/handoff/research
```

Non riportare in vita:

- roster obsoleti;
- tempi reaction obsoleti;
- mappe quadrate/flat obsolete;
- nomi personaggi superati;
- architettura PDR scaduta;

solo perché compaiono in un PDF.

---

# 20. Validazione visuale delle sette immagini

Dopo averle importate:

- aprirle realmente;
- controllare leggibilità a 1080p;
- controllare termini inglese/italiano;
- controllare issue ID;
- controllare status;
- controllare che nessun diagramma presenti una decisione aperta come canonica;
- controllare che i link/caption spieghino `draft` vs `canonical` vs `snapshot`.

In particolare:

```text
F1/F2 -> controllare FAC-12
F3    -> canonical after FAC-11
F4    -> controllare #164/#165 current state
F5    -> FAC-14 / proposed
F6    -> privacy canonical
F7    -> dated status snapshot
```

---

# 21. Commit strategy

Preferire commit separati:

```text
docs(facing): formalize six-side relative direction model
docs(facing): reconcile ADRs open decisions and conflict matrix
docs(roadmap): align facing issues scenarios and feature gates
docs(wiki): add facing flow visual assets
docs(wiki): update facing movement overwatch and privacy pages
test(facing): add missing executable facing scenarios
docs(issues): reconcile facing and ghost timeline ownership
```

Se GitHub issue body e Wiki clone sono fuori dal commit del main repository, elencarli nel report finale.

---

# 22. Verifiche richieste

Eseguire i checker reali del repository.

Almeno:

```bash
python scripts/feature_registry.py --help
python scripts/check-docs-naming.py
```

Poi gli specifici validator/linters trovati in CI.

Per gli scenari:

- validare indice;
- eseguire la suite scenario disponibile;
- distinguere test **dichiarati** da test **eseguiti**;
- se si aggiunge uno scenario BLOCKED, spiegare capability mancante;
- se si rende uno scenario GREEN, allegare evidenza di esecuzione.

Non dichiarare `GREEN` soltanto perché il JSON parsea.

---

# 23. Output finale obbligatorio di Claude

Alla fine produrre un report con questa struttura:

```text
HEAD iniziale / HEAD finale

Decisioni:
- FAC-11: ...
- altre FAC cambiate: ...

Documentazione:
- file creati
- file modificati
- file archiviati
- conflitti risolti

Wiki:
- clone aggiornato
- pagine aggiornate
- immagini usate per pagina
- deploy feature status eseguito sì/no

Asset:
- directory
- 7 file copiati
- hash verificati
- eventuali diagrammi marcati draft/snapshot

Roadmap:
- epic/CP aggiornati
- prossima issue reale
- dipendenze cambiate

GitHub:
- issue aggiornate
- issue create
- issue chiuse
- nessun duplicato creato

Scenario:
- scenari già esistenti trovati
- scenari nuovi
- scenari modificati
- BLOCKED con motivazione
- suite eseguite

Feature Registry:
- gate prima/dopo
- generated files rigenerati

Editor Map:
- delta

Privacy:
- test/canary verificati

TurnLog / replay:
- impatto oppure "nessun impatto"

Rischi / decisioni ancora aperte:
- FAC-12
- FAC-3 / FAC-5... se ancora aperte
- altri gap misurati

Prossimo lavoro raccomandato:
- UNA issue
- perché è la prossima
- quale scenario la dimostra
```

---

# 24. Divieti

Non:

- creare una nuova epic Facing se E16 copre già il dominio;
- creare una seconda pagina Wiki Facing;
- creare pagine Wiki dentro `docs/wiki/`;
- modificare generated registry files a mano se esiste generator;
- cambiare Guard/Brace come effetto collaterale di FAC-11;
- chiudere FAC-12 usando il testo di F1/F2;
- segnare F7 come live dashboard;
- riaprire #164 se è chiusa;
- lasciare #172 con "Facing non autorevole";
- aggiungere planned Facing avversario a DTO pubblici;
- usare Actor rotation / animazione come autorità;
- duplicare path/LOS/targeting nel renderer;
- creare scenari duplicati solo con nomi nuovi;
- far diventare l'harness più capace dell'input reale senza dichiararlo;
- copiare PDR storici sopra ADR correnti.

---

# 25. Mappa consigliata immagini → Wiki

| Immagine | Pagina primaria | Pagina secondaria |
|---|---|---|
| F1 | `facing-e-direzionalita` | turno / simulazione |
| F2 | `facing-e-direzionalita` | movimento |
| F3 | `facing-e-direzionalita` | targeting / difese |
| F4 | Overwatch / reazioni | Facing |
| F5 | movimento forzato / control | Facing — sezione proposed |
| F6 | privacy / planning | Facing / Ghost Timeline |
| F7 | roadmap / scenario coverage | Facing — link allo stato |

Se i nomi reali delle pagine Wiki differiscono, usare quelli già esistenti.
**Non creare alias/duplicati senza necessità.**

---

# 26. Risultato atteso

Alla chiusura di questo apply:

- la decisione "Facing = sei lati espliciti dell'hex" è formalizzata;
- FAC-11 non resta ambiguo;
- #339 racconta lo stato reale;
- #172 non contraddice più Facing autorevole;
- roadmap/registry/scenario map concordano;
- la Wiki Facing è aggiornata;
- le pagine correlate sono cross-linkate;
- i sette visual sono archiviati e usati con il corretto livello di autorità;
- F7 è dichiarata snapshot datata;
- gli scenari mancanti sono stati creati **solo se realmente mancanti e implementabili**;
- non è nata una seconda fonte di verità.


# 27. Hash SHA-256 degli asset consegnati

Usali per verificare che i file copiati siano esattamente quelli del pacchetto.

| File | SHA-256 | Bytes |
|---|---|---:|
| `F1_Lifecycle_del_Facing_nel_Turno.png` | `5f9ad7b593051a4f76fe9f1276791effd38ea5022501508dd162a8a2708e81f1` | 1629889 |
| `F2_Move_Micro_step_e_Pivot.png` | `1a794a1578f2dffd25d99878c7cb75254f5cc803f87d6844718b387f0f4a7a37` | 1557094 |
| `F3_Attacco_e_Direzione_Relativa.png` | `679e9531a44a239c2888f8c73c911780cec24c96a479b69928b4a8bdd056287f` | 1751974 |
| `F4_Overwatch_Reaction_Facing.png` | `1eb432430eeac32c618dfcf5995a1fbf682dbb99cf149d3eb776ddcec74d957e` | 1662691 |
| `F5_Forced_Movement_Facing_Control.png` | `0afda3be5c21d27a9db745c043b8247978160e1f19d996d62131d77d87453ee9` | 1711825 |
| `F6_Privacy_Team_only_UI_del_Facing.png` | `20184b831e0234575659eb7d28bd5e4adc97eee8b68793595d03850dd5c5a0fb` | 1469340 |
| `F7_Scenario_Coverage_Map_Facing.png` | `c108476c077d2b4185e392af2bc5d3f20508bd365e6f7e4dcda742a28662dc08` | 1630793 |


# 28. Comando finale di sanity check suggerito

```bash
git diff --check
git status --short

# poi i checker del repository e la suite interessata
# infine confrontare:
# roadmap <-> feature registry <-> scenario map <-> issue <-> Wiki
```

Se due fonti canoniche restano in conflitto, non scegliere in silenzio:
registrare il conflitto e fermare soltanto il delta che dipende da quella decisione.
