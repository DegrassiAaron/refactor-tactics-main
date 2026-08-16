> `ARCHIVIATO` · **2026-08-16** · Consumato in due giri. **Non è autorità**: le sue premesse descrittive
> erano già invecchiate al momento del consumo, e la revisione che lo misura è
> [`../../roadmap/plans/spec-panel-reconcile-2026-08-16.md`](../../roadmap/plans/spec-panel-reconcile-2026-08-16.md).
>
> **Consumato**: §0 (misura) · §8 (con la **direzione corretta**: il codice aveva migrato, i documenti no) ·
> §11 limitatamente a ciò che il §8 tocca · §12 (audit delle claim falsificabili, primo giro) · §13 (tracking
> e gate) · §16 per il residuo.
>
> **NON consumato, e ciascuno per una ragione dichiarata**:
> · **§2 su #995** — track `proficiency` `ACTIVE`: STOP per D-139, non una lacuna.
> · **§2 su #1006** — issue `CLOSED`, opzione C decisa e implementata (PR #1008): applicarlo sarebbe un regresso.
> · **§5, §6, §9, §10** — lavoro di gameplay e decisioni d'autore, non riconciliazione documentale. Il §10 lo
>   dice da sé: *«aggiorna queste issue solo se il contratto riconciliato cambia una loro premessa»*, e non la cambia.
> · **La scheda §12 completa per 23 issue** — perimetro escluso per costo, in entrambi i giri.
>
> ⚠️ **Cinque delle sue premesse erano cadute prima di essere lette** — #1006, #63, #583, #152, #403 — tre
> nella stessa giornata. È il motivo per cui un kit si **filtra** e non si applica.

# CLAUDE — Reconcile v0.1 Skill / Ability Issues
**Project:** RefactorTactics  
**Date:** 2026-08-16  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Scope:** GitHub issues + canonical docs + roadmap/feature/scenario tracking related to skills, abilities, reactions, equipment, elemental access and their immediate UI/replay consumers.

---

## Missione

Riconcilia le issue aperte della **v0.1** che toccano skill/abilità con lo stato reale di `main` e con le decisioni più recenti.

Non fare un redesign del sistema abilità. Il lavoro principale è:

1. eliminare blocker e dipendenze stale;
2. eliminare contraddizioni fra issue, codice, Decision Log, owner docs, roadmap e Feature Registry;
3. rendere ogni issue implementabile e falsificabile;
4. evitare duplicati e seconde pipeline;
5. separare chiaramente:
   - decisioni d'autore ancora aperte;
   - lavoro di gameplay;
   - lavoro data-only;
   - lavoro UI;
   - lavoro replay/determinismo;
   - balance da playtest;
6. aggiornare tracking e documentazione derivata dopo ogni correzione.

**Regola fondamentale:** non "correggere" una issue basandoti sulla prosa vecchia. Prima misura `main`, issue collegate, Decision Log e owner documentale.

---

# 0. Prima di modificare

## 0.1 Sincronizza e misura

```bash
git status
git fetch origin
git switch main
git pull --ff-only
gh repo view
gh issue list --state open --label v0.1 --limit 500
```

Controlla anche PR aperte:

```bash
gh pr list --state open --limit 200
```

## 0.2 Rispetta D-139 / parallel batch

Prima di toccare file:

- leggi `docs/roadmap/parallel-batch.yaml`;
- assegna il write-set corretto;
- se un file è posseduto da un'altra track: **STOP**, non fare merge conflict "temporanei";
- `integration_only` significa che la track prepara il delta ma non lo scrive direttamente.

## 0.3 Non inventare ID decisione

Se serve una nuova decisione:

```bash
python scripts/rt_shared_id.py reserve D
```

Mai scegliere `D-nnn` a mano.

---

# 1. Baseline architetturale da NON rompere

Per la **v0.1**:

- niente GAS come dipendenza necessaria al runtime delle skill;
- C++ resta autorità di simulazione, validazione, determinismo e TurnLog;
- dati/cataloghi dichiarano le varianti;
- UI non ricalcola regole;
- client/UI propongono, resolver decide;
- niente callback speciali per eroe se l'effetto può essere espresso dai dati;
- niente seconda pipeline per Reaction, Predictive, Equipment o Elemental;
- ID stabili e migrazioni di naming possedute dalle issue dedicate;
- replay/verifier deve riprodurre gli input canonici, non chiedere di nuovo decisioni umane;
- nessuna informazione privata avversaria deve entrare nei DTO/UI.

Pipeline target:

```text
Definition / Catalog
        ↓
Planning Intent
        ↓
Validation
        ↓
Immutable Snapshot
        ↓
Authoritative Resolver
        ↓
TurnLog / Canonical Events
        ↓
Presentation / Replay / UI
```

Per le reaction:

```text
Prepared Reaction
    ↓
Reaction Opportunity
    ↓
AllowedResponses
    ├─ 0 → nessuna decisione
    ├─ 1 → commit automatico
    └─ 2+ → Decision Boundary / Decision Window
                    ↓
              Canonical Decision
                    ↓
                 Resolver
```

`Reaction Clash` deve essere un caso derivato della stessa pipeline, non un secondo sistema.

---

# 2. Issue da riconciliare

## Gruppo A — Elemental / hero abilities

### #995 — Elemental Proficiency v0.1
**Intento valido:**
- due assi distinti:
  - `Affinity / Weakness` = identità dichiarata;
  - Elemental Proficiency = derivata dagli effetti reali del kit.
- criterio operativo basato sui dati:
  - `FRTActionEffectSpec`;
  - `bCreatesSurface`;
  - Generate / Apply / Propagate / Transform / Consume;
  - non nome, VFX o `DamageType`.

**Baseline d'autore da preservare finché non viene esplicitamente riaperta:**
- Gadget = Electric `Specialist`;
- Phase = Water `Access`;
- Riktor = None elementale, con `Affinity.Structures`;
- Wraith = None elementale, con `Affinity.Movement`.

### Azione
1. verifica il catalogo reale;
2. ricostruisci la tabella ability → effect data → funzione elementale;
3. aggiorna #995 se contiene ancora affermazioni non misurate;
4. non duplicare la matrice in più owner;
5. chiarisci definitivamente il ruolo di `Signature/Profile Equipment`:
   - se non è rappresentabile nei dati, non lasciare una regola normativa non eseguibile.

---

### #1006 — Phase Water Access
Stato noto da verificare su `main`:

```text
PressureJet  -> Wet + Push       -> Apply Water
CircularTide -> Wet + Heal       -> Apply Water
FluidTrail   -> CreateWater      -> Generate Water
MistVeil     -> Smoke            -> non Water
```

Con la grammatica di #995 questo rende Phase `Master`, mentre il target approvato è `Access`.

## Vincoli
- `Phase = Water Access` non è una semplice etichetta: il catalogo deve produrre davvero quel grado.
- `Gadget = Electric Specialist` non deve essere degradato incidentalmente.
- `FluidTrail` è legata a D-046 / `Action.CreateWater`.
- `Gadget.Sprinkler` è già una sorgente alternativa di acqua: misurare se mantiene innescabili gli scenari/CP ambientali prima di cambiare ownership.

## Revisione richiesta
Le opzioni A/B/C/D della issue non devono restare una falsa scelta se, mantenendo tutte le decisioni già approvate, solo una è realmente coerente.

Verifica esplicitamente:

- A: togliere Water a `FluidTrail` → Phase resta Specialist?
- B: togliere Wet a `CircularTide` → Phase resta Specialist?
- C: A+B → Phase diventa Access?
- D: escludere le core-hosted capability → degrada anche Gadget?

Se la misura conferma questa matrice, riscrivi #1006 distinguendo:

```text
APPROVED CONSTRAINTS
MEASURED CONSEQUENCES
AUTHOR DECISION REQUIRED / NOT REQUIRED
IMPLEMENTATION PLAN
```

**Non canonizzare una soluzione che richieda una nuova decisione d'autore senza segnalarlo.**

## Scenario da verificare
`Scenarios/Spec/Environment/WaterQuenchesFire.json`

Verifica se può usare `Gadget.Sprinkler` senza perdere il significato dello scenario.

---

# 3. Equipment / loadout / ability variants

### #61 — CP 7.2 Gadget
Otto gadget v0.1.

Punto da consolidare:
- `Sprinkler` è ora rilevante anche come accesso esterno all'acqua;
- non trasformare questa issue nella owner della Elemental Proficiency;
- il gadget dichiara effetti nei dati, la proficiency dell'eroe resta una derivazione separata.

### #63 — CP 7.4 Loadout 1+1+1
Stato verificato su GitHub:
- open;
- v0.1 / P2;
- `1 Weapon Variant + 1 Gadget + 1 Reaction Module`.

Problema da controllare:
i default usano ancora i nomi runtime/storici `Flux/Riva/Bastion/Vektor`.

Non rinominare token opportunisticamente qui se la migrazione è posseduta dalle issue dedicate.

Aggiorna la prosa player-facing solo se il naming owner lo consente.

### #509 — CP 7.6 Damage Bands
Stato verificato su GitHub:
- open;
- v0.1 / P2;
- `DamageDelta` deve diventare `DamageDeltaByBand`.

Vincolo:
- fascia derivata dal **base damage della definition**;
- mai da Wet/buff/debuff/runtime;
- numeri restano `PROPOSED FOR PLAYTEST`;
- nessun branch per eroe.

## Consolidamento comune #61/#63/#509
Assicurati che il modello dati resti:

```text
Hero kit
Equipment
Weapon Variant
Gadget
Reaction Module
```

senza introdurre:
- upgrade puri;
- rarity;
- progressione in-match;
- moltiplicatori float;
- regole eroe-specifiche nel core.

---

# 4. Reaction Tactics System

## Regola da consolidare
Brace, Overwatch, Fast Reaction e Reaction Clash sono parti dello **stesso Reaction Tactics System**.

Non creare:
- `BraceResolver`;
- `OverwatchResolver`;
- `FastReactionResolver`;
- `ClashResolver`

come autorità indipendenti.

Le differenze devono vivere in:
- trigger;
- profilo;
- allowed responses;
- effect specs;
- policy/costi;
- dati.

---

## #152 — E14 Overwatch e reazioni interattive

Verifica e correggi eventuale testo stale.

La issue dichiara ancora che E14 è "l'ultima epic della v0.1", ma oggi esistono almeno E46 ed E47.

Non cambiare la priorità tecnica solo per questo: correggi la frase in modo che non menta sullo stato reale della roadmap.

Sequenza da preservare:

```text
14.5 ✅
  ↓
14.6 #166
  ↓
14.7 #314 (P3 extension)
  ↓
14.8 #319 (P3 extension)
```

`#314` e `#319` restano estensioni tagliabili: non devono diventare prerequisito implicito della baseline reaction già funzionante.

---

## #166 — Counterplay + UI + pacing

Questa è la prossima issue baseline di E14.

Deve coprire:
- Stun / KO / Disarm / forced movement invalidano la reaction quando previsto;
- UI `FIRE/HOLD`;
- countdown reale;
- `FastReactionDuration`;
- timeout → HOLD;
- slow motion solo presentation;
- DTO sanitizzato;
- privacy;
- misura `ReactionDecisionSeconds`;
- p50/p90;
- campione dichiarato;
- separazione da `ResolutionPlaybackSeconds`.

## Dipendenza critica da rendere esplicita
**#886 deve atterrare prima o insieme a #166.**

Non chiudere #166 con una UI umana funzionante se il Verifier non sa riprodurre la decisione registrata.

---

## #886 — Reaction decisions nel Replay Verifier

Correggere il replay/verifier affinché:

```text
OpportunityId → Recorded Decision
```

sia input canonico durante la ri-simulazione.

Requisiti:
- decisione registrata batte qualunque live decider;
- il live decider non viene interrogato;
- una decisione non può migrare a una opportunity diversa;
- scenario con vera reaction decision nel determinism corpus.

Questa è infrastruttura di correttezza, non UI.

---

## #583 — Declared Reaction Condition

**Trovato stale blocker.**

La issue dice ancora in più punti di essere bloccata da #165.

Ma #165 è chiusa.

### Azione obbligatoria
1. verifica su `main` cosa #165 ha realmente consegnato;
2. rimuovi/riscrivi ogni:
   - "blocked by #165";
   - "non scrivibile finché #165";
   - "BuildOverwatchTriggers solo test";
   se non è più vero;
3. completa o ridefinisci le due DoD residue:
   - Intent → Planning → Snapshot → Resolver → TurnLog;
   - condition registrata come dato per replay;
4. controlla se parte del lavoro è ora assorbito da #886;
5. se sì:
   - non duplicare;
   - rendi #583 dipendente dal seam corretto o riduci il DoD.

**Non lasciare una issue aperta con una dipendenza chiusa che la dichiara ancora "bloccata".**

---

# 5. Reaction Profile / Clash / Time Bank

## #314 — Reaction Profile + Reaction Clash

Mantieni:

```text
Brace -> arms Reaction Profile
Hold Ground -> universal baseline response
```

Profili correnti da verificare:
- Gadget → `Profile.Grounding`;
- Phase → `Profile.Sidestep`;
- Wraith → `Profile.Glance`;
- Riktor → nessun profilo aggiuntivo se `Hold Ground` copre già il comportamento.

`Contested` deve restare **derivato**:

```text
participant A has >= 2 legal responses
AND
participant B has >= 2 legal responses
```

Niente `Type = Clash`.

Clash:
- blind choice;
- fixed-deadline reveal;
- un solo prompt condiviso;
- no nested Clash;
- cost consumed at valid lock secondo policy;
- outcome via `FRTActionEffectSpec`.

La grammatica `STAND / READ / SHIFT` resta `PROPOSED FOR PLAYTEST` finché non promossa.

---

## #319 — Decision Time Bank

Non anticiparla.

Deve venire dopo la misura reale di #166 e dopo #314 se la roadmap mantiene la sequenza attuale.

Non fissare `Grace`, `InitialBank`, ecc. "a occhio".

Controlla che:
- owner-only;
- canonical recorded input;
- no effect sulle legal responses;
- replay non ricalcola dal timer;
- contested fixed-deadline non venga falsamente descritto come pacing reduction.

---

# 6. Decision Boundary + cover/facing

## #888 — Overwatch/Predictive ignorano cover?

Questa issue è una **decisione d'autore aperta**.

Non chiuderla scegliendo in autonomia.

Il problema deve essere reso leggibile come scelta di regola comune ai Decision Boundary:

```text
A) Boundary attack ignores cover/facing
B) Boundary attack applies normal combat geometry
```

Se B:
serve decidere quale stato/cella è autorità per la geometria al boundary.

La soluzione tecnica preferibile, **solo come proposta da sottoporre all'autore**, è usare lo stato canonico del micro-step nel momento del Decision Boundary e gli stessi servizi di LOS/cover/facing del combat ordinario.

Ma non promuoverla a decisione senza approvazione.

## DoD della issue
La issue deve terminare con:
- opzioni;
- conseguenze gameplay;
- conseguenze resolver;
- conseguenze UI;
- test richiesti;
- decisione d'autore esplicita.

Non deve restare una domanda vaga.

---

# 7. Guard / Brace balance

## #403 — BAL-1
## #404 — applicazione BAL-1

Mantienile separate da E14.

E14 costruisce il sistema reaction.
BAL-1 decide il **mestiere e i numeri** di Guard/Brace tramite playtest.

Non cambiare numeri di Brace per "far funzionare" Reaction Profile.

Sequenza:

```text
scenario + playtest
    ↓
#403 author decision
    ↓
Decision Log
    ↓
#404 implementation + tests + docs
```

---

# 8. Predictive vs Reaction

Verifica che nessun documento/issue recente abbia ri-trasformato `InterceptShot` in Reaction.

Decisione corrente:
- `InterceptShot` è **Predictive Action**;
- E18 thin slice risulta chiusa;
- Wraith `Profile.Glance` è invece Reaction Profile.

Questi due concetti non devono essere fusi.

Cerca:

```bash
grep -Rni "InterceptShot" Source docs Scenarios
grep -Rni "Profile.Glance" Source docs Scenarios
grep -Rni "Reaction.*Intercept\|Intercept.*Reaction" Source docs Scenarios
```

Se trovi prosa CURRENT che li confonde:
- correggi owner;
- archivia sorgenti superate secondo le regole del repo;
- non modificare HISTORICAL/SNAPSHOT come se fossero CURRENT.

---

# 9. Noise come proprietà delle azioni

## #690 — Noise intensity
Questa issue deve restare **data-only**.

Porta il dato nel catalogo.
Non implementa perception runtime.

Verifica:
- azioni generiche;
- abilità firma: decidere se hanno intensità propria o una policy esplicita;
- nessun valore resta solo in commento C++;
- nessun hardcode duplicato nel producer.

## #686 — Hearing threshold
È il lato receiver della stessa scala.

Assicurati che catalogo eroi e runtime coincidano.

## #159 — Runtime acoustic contact
È il consumer runtime.

Separazione corretta:

```text
#690 → source data
#686 → receiver data
#159 → emission + propagation + team-filtered knowledge
```

Non spostare il producer runtime in #690.

Privacy:
- noise deriva da un evento realmente risolto;
- mai da un planned enemy intent;
- TurnLog completo;
- observer/team filtering in uscita.

---

# 10. UI consumers delle skill

Verifica, senza allargare scope, le issue:

- #77 — HUD action slots;
- #78 — certainty;
- #172 — phase ghosts;
- #173 — reaction as conditional branch;
- #613 — UMG screen HUD;
- #705 — pointer/reaction input precedence.

Principi:
- reaction non è una quinta fase;
- UI non calcola legalità;
- reaction UI consuma un DTO sanitizzato;
- planned enemy reaction/facing/target non deve arrivare al client;
- Action Dock e ghost devono usare semantic IDs/catalog;
- niente texture hardcoded se il catalogo icone è l'owner.

Aggiorna queste issue **solo** se il contratto skill/reaction riconciliato cambia una loro premessa.

---

# 11. Naming migration

Il progetto sta migrando dai nomi storici:

```text
Flux / Riva / Bastion / Vektor
```

ai nomi correnti:

```text
Gadget / Phase / Riktor / Wraith
```

Le migrazioni hanno issue dedicate (#753…#757 secondo il tracking corrente).

Regola:
- non fare rename opportunistici dentro #61, #63, #1006, #314 ecc.;
- non introdurre nuovi token col naming vecchio;
- nei documenti CURRENT player-facing usare il naming corrente se consentito dal naming gate;
- i token runtime ancora non migrati vanno citati come token tecnici, non scambiati per nome canonico del personaggio.

Esegui:

```bash
python scripts/check-docs-naming.py --check
```

---

# 12. Audit delle issue: cosa sistemare materialmente

Per ogni issue toccata crea una scheda locale:

```text
ISSUE:
STATE:
LABELS:
MILESTONE:
OWNER DOC:
DECISIONS:
CODE REALITY:
STALE CLAIMS:
REAL DEPENDENCIES:
AUTHOR DECISION NEEDED:
IMPLEMENTABLE NOW:
DOD CHANGES:
TESTS:
TRACKING CHANGES:
```

Poi aggiorna l'issue GitHub.

## Non limitarti ai commenti
Se il body CURRENT mente:
- aggiorna il body;
- conserva la storia nei commenti/git dove serve;
- non aggiungere un altro commento che contraddice il body lasciandolo falso.

---

# 13. Tracking da riconciliare

Controlla almeno:

```text
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/feature-registry.yaml
docs/roadmap/scenariomap.shortlist.md
docs/technical/scenario-map.md
docs/OPEN_DECISIONS.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/gameplay/
docs/characters/
docs/balance/
```

Più gli owner citati dalle issue.

## Feature Registry
Dopo modifica:

```bash
python scripts/feature_registry.py validate
```

Se una vista derivata è cambiata, rigenerala col comando previsto dal repo.

## Docs gates
Esegui i gate disponibili in `AGENTS.md`.

Minimo:

```bash
python scripts/check-docs-naming.py --check
python scripts/check-docs-links.py --check
python scripts/check-docs-symbols.py --check
python scripts/feature_registry.py validate
```

Se esiste ormai il gate per stale dependencies, eseguilo.
Se #738 è ancora aperta e il gate non esiste, non inventare che sia verde.

---

# 14. Issue nuove: criterio severo

Prima di creare una issue:

```bash
gh search issues "<keyword>" --repo DegrassiAaron/refactor-tactics-main --state open
gh search issues "<keyword>" --repo DegrassiAaron/refactor-tactics-main --state closed
```

Crea una issue nuova solo se:
- non esiste owner;
- non è una sotto-voce naturale di una issue aperta;
- ha un DoD autonomo;
- non duplica una decisione d'autore già tracciata.

Preferisci correggere una issue stale invece di crearne una gemella.

---

# 15. Decisioni che NON devi prendere in autonomia

Fermati e produci una sezione `AUTHOR INPUT REQUIRED` se, dopo la misura, resta una di queste:

1. #888 — cover/facing sui Decision Boundary;
2. #403 — confine Guard vs Brace;
3. numeri di balance `WV-2`;
4. valori non promossi del Decision Time Bank;
5. qualunque modifica che contraddica una decisione `Consolidata`;
6. qualunque scelta che trasformi Phase da `Access` in altro grado;
7. qualunque scelta che trasformi Gadget Electric da `Specialist` senza decisione esplicita;
8. qualunque nuova classificazione Signature/Generic Equipment che richieda un campo runtime nuovo.

Puoi preparare opzioni e conseguenze, non scegliere al posto dell'autore.

---

# 16. Priorità operativa

Ordine consigliato per la riconciliazione:

```text
A. audit + stale claims
B. #995 / #1006
C. #583 stale blocker
D. #886 + #166 dependency
E. #152 wording / E14 scope
F. #888 decision brief
G. #61 / #63 / #509 consistency
H. #690 / #686 / #159 boundaries
I. #314 / #319 dependency and cut-line
J. UI consumers only where impacted
K. roadmap / registry / scenario map / open decisions
L. validation
```

Per implementazione gameplay, non confondere quest'ordine con una richiesta di fare tutto in una PR.

Preferisci PR piccole e semanticamente coese.

---

# 17. Definition of Done globale

Il lavoro è finito quando:

- [ ] nessuna issue CURRENT dichiara un blocker già chiuso;
- [ ] #583 non dipende più falsamente da #165;
- [ ] #152 non dichiara più falsamente di essere l'ultima epic se la roadmap corrente lo smentisce;
- [ ] #995 e #1006 descrivono la stessa grammatica e lo stesso target di Phase;
- [ ] le opzioni di #1006 sono ridotte alle vere scelte rimaste dopo la misura;
- [ ] #166 dichiara #886 come prerequisito/co-delivery dove necessario;
- [ ] #314/#319 non bloccano implicitamente la baseline reaction se restano P3;
- [ ] #888 è una decision issue falsificabile e non una domanda vaga;
- [ ] #403/#404 restano owner del balance Guard/Brace;
- [ ] #690 resta data-only e #159 resta runtime consumer;
- [ ] Predictive e Reaction non sono nuovamente mescolati;
- [ ] nessun nuovo hardcode eroe-specifico è stato introdotto;
- [ ] nessun nuovo leak di planning avversario è possibile;
- [ ] roadmap, feature registry, scenario map e open decisions sono coerenti;
- [ ] tutti i docs gates applicabili sono verdi;
- [ ] suite automatica pertinente è verde;
- [ ] ogni modifica ha evidenza "before → after";
- [ ] nessuna issue duplicata è stata creata.

---

# 18. Output finale richiesto a Claude

Alla fine restituisci:

## A. Executive summary
Massimo 15 righe.

## B. Issue matrix

| Issue | Before | After | Action | Decision needed | PR/commit |
|---|---|---|---|---|---|

## C. Stale claims removed
Elenco puntuale di ogni affermazione rimossa/corretta e prova che era stale.

## D. Decisions still requiring author
Solo decisioni vere, con massimo 2–3 opzioni ciascuna e conseguenze.

## E. Files changed
Con motivo.

## F. GitHub issues changed
Con link/numero e sintesi del body aggiornato.

## G. Validation
Riporta i comandi realmente eseguiti e il risultato reale.

Non scrivere "all green" se non hai eseguito il comando.

## H. Next issue
Indica **una sola** issue successiva consigliata per continuare l'implementazione delle skill, motivata dalle dipendenze reali dopo la riconciliazione.

---

# 19. Commit suggeriti

Non forzare questi confini se il write-set li rende peggiori, ma preferisci:

```text
docs(abilities): reconcile elemental proficiency and Phase access issues
docs(reactions): remove stale blockers and align E14 dependencies
docs(tracking): reconcile skill issue roadmap and feature registry
test(reactions): align replay decision contracts with current issues
```

Ogni commit deve essere reversibile e non mescolare balance, runtime e docs senza motivo.

---

# 20. Regola finale

**Misura prima, modifica dopo.**

Se issue, documentazione e codice si contraddicono:

1. identifica l'owner;
2. identifica la decisione più recente valida;
3. misura il runtime;
4. classifica la divergenza;
5. correggi tracking/documentazione oppure apri una vera decisione;
6. non nascondere la contraddizione con una frase più vaga.

L'obiettivo non è far sembrare ordinate le issue.

L'obiettivo è fare in modo che la prossima issue implementata abbia **una sola interpretazione tecnica possibile**, salvo le decisioni d'autore esplicitamente ancora aperte.
