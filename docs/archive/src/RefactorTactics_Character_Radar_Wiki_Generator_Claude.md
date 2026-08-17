# RefactorTactics — Character Radar & Wiki Visualization Pipeline
> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

## Handoff operativo per Claude Code

**Obiettivo:** consolidare nel repository RefactorTactics le decisioni emerse sul modo di rappresentare e confrontare i personaggi tramite radar/spider chart, definire la pipeline dati → grafici → Wiki, aggiornare documentazione e tracker di progetto, e creare/aggiornare Epic e Issue senza duplicare materiale già esistente.

# 0. ISTRUZIONI OPERATIVE PER CLAUDE

Lavora sul repository corrente di RefactorTactics.

Prima di modificare file:

1. Ispeziona la struttura reale del repository.
2. Individua i file già esistenti per Wiki, documentazione/PDR, Roadmap, Feature Map, Scenario Map, Editor Map, eventuali tracker YAML, dati personaggi/balance, tooling docs/wiki, GitHub issue templates, labels, milestones ed Epic.
3. Aggiorna e consolida i file esistenti invece di crearne duplicati.
4. Se i path proposti in questo documento non coincidono con il repository, usa i path reali e documenta la scelta.
5. Verifica le decisioni contro le fonti canoniche già presenti nel repo, in particolare PDR personaggi/abilità/GAS, PDR UI/UX, PDR dati/validazione/modding-ready, PDR roadmap/QA e balance matrix / character data correnti.
6. Non rendere canonici valori numerici che in questa specifica sono solo esempi.
7. Mantieni stabile il principio: **C++/dati competitivi definiscono il gameplay; i radar sono una rappresentazione derivata per UI, Wiki e analisi.**
8. Non modificare regole di gameplay o valori competitivi solo per far “apparire meglio” un radar.
9. Esegui test/validator applicabili e lascia il repository in stato coerente.
10. Al termine produci un riepilogo con file creati/modificati, Epic/Issue create o aggiornate, tracker aggiornati, decisioni consolidate, punti ancora aperti, test eseguiti e commit suggeriti o creati secondo il workflow del repository.

# 1. DECISIONE DI DESIGN CONSOLIDATA

Per rappresentare rapidamente il profilo tattico di un personaggio useremo un **Radar Chart**, alias **Spider Chart** o **diagramma a ragnatela**.

Ogni raggio rappresenta una **dimensione statistica**, non una singola abilità.

Il poligono risultante deve permettere di leggere rapidamente la “forma tattica” di un personaggio e confrontarla con quella di altri personaggi o build.

Obiettivi:

- rendere il roster leggibile nella Wiki;
- confrontare personaggi;
- confrontare build/varianti;
- facilitare discussioni di balance;
- evidenziare identità e trade-off;
- evitare tabelle numeriche troppo dense come unica forma di comunicazione.

# 2. SEPARAZIONE OBBLIGATORIA: STATS REALI VS RATING RADAR

Questa distinzione è fondamentale.

## 2.1 Statistiche reali di simulazione

Esempi: HP, Armor, Movement, Range, cooldown, costi, displacement, durata status, Detection/Hearing reali, ecc.

Sono dati utilizzati dal gameplay e dal simulatore.

## 2.2 Rating sintetici del radar

Sono valori normalizzati, inizialmente in scala `1..10`.

Servono a visualizzazione, Wiki, confronto, design review e balance overview.

**NON devono essere usati direttamente dal resolver.**

Non inserire direttamente nello stesso radar valori non comparabili come HP=120, Move=5, Range=7, Detection=40. I radar devono usare dimensioni normalizzate.

# 3. DUE RADAR DISTINTI

## 3.1 Character Profile Radar — pubblico / Wiki / design

Assi consolidati:

1. Offesa
2. Durabilità
3. Mobilità
4. Controllo
5. Supporto
6. Informazione

Chiavi dati consigliate:

```text
offense
durability
mobility
control
support
information
```

### Offesa
Pressione, danno, capacità di finire bersagli, affidabilità offensiva, output con e senza setup.

### Durabilità
Resistenza, mitigazione, scudi, self-protection, capacità di sostenere focus.

### Mobilità
Movement, dash, reposition, libertà di percorso, engage/escape, cambio quota/rotta quando applicabile.

### Controllo
Push, pull, slow, stun/control, zoning, denial, modifica cover, modifica archi/transizioni, manipolazione terreno, blocco/canalizzazione rotte.

### Supporto
Protezione alleati, heal/sustain, buff, peel, setup per combo, utility di squadra.

### Informazione
Reveal, detection, stealth, percezione, rumore, tracking, visibilità/LOS, negazione/manipolazione informativa.

Nota: Reaction/Overwatch non è per ora un asse dedicato. Va rappresentato con tag/iconografia finché non emerge come dimensione universale del roster.

## 3.2 Balance Radar — interno / design / tuning

Assi:

1. Precisione
2. Potenza
3. Controllo
4. Supporto
5. Durabilità

Chiavi:

```text
precision
power
control
support
durability
```

Questo radar è più vicino alle matrici di bilanciamento interne.

# 4. SCALA 1–10

Baseline:

| Valore | Interpretazione |
|---:|---|
| 1–2 | Molto debole |
| 3–4 | Sotto media |
| 5–6 | Medio |
| 7–8 | Forte |
| 9–10 | Eccellente / tratto distintivo |

Regole:

- evitare personaggi pieni di 9–10;
- evitare personaggi completamente piatti;
- la forma deve comunicare trade-off reali;
- un 10 deve rappresentare un elemento fortemente identitario;
- i rating non vanno assegnati “a vibe” quando possono essere derivati o verificati con metriche;
- definire in futuro una rubrica riproducibile per convertire stats e capacità del kit in rating.

# 5. IMPORTANTE: VALORI CANONICI

Durante la discussione sono stati usati esempi numerici per mostrare il sistema.

**NON assumere che siano valori canonici.**

In particolare Gadget, Phase, Riktor e Wraith devono essere valorizzati solo dopo verifica contro i dati correnti del repository / balance matrix.

Se il repository possiede già rating equivalenti:

1. riusarli;
2. documentare il mapping;
3. evitare una seconda fonte di verità.

Se alcuni rating non esistono ancora:

- lasciare TBD/null o equivalente previsto dallo schema;
- oppure creare una Issue di design/balance per definirli;
- non inventarli durante il consolidamento.

# 6. GENERATORE WIKI

Serve un generatore automatico.

```text
Character Data
      |
      v
Validation
      |
      v
Wiki Chart Generator
      |
      +--> Single Character Radar
      |
      +--> Character Comparison Radar
      |
      v
Generated SVG
      |
      v
Wiki
```

Principio: **le immagini non sono source of truth. Devono essere output generati.**

# 7. TECNOLOGIA MVP

Soluzione proposta:

- Node.js 20+
- TypeScript
- YAML come input iniziale se non esiste già una fonte dati equivalente nel repo
- SVG come formato primario

Motivi:

- SVG scalabile;
- leggibile in browser/wiki;
- leggero;
- facilmente versionabile;
- deterministico;
- semplice da integrare in GitHub Actions in futuro.

Se il repository ha già un toolchain differente equivalente, integrare lì invece di introdurne una nuova senza motivo.

# 8. SOURCE OF TRUTH E STRATEGIA DATI

Preferenza architetturale:

```text
Gameplay/Balance canonical data
        |
        v
normalized/derived wiki view data
        |
        v
chart generator
```

Per l’MVP può essere accettabile un YAML dedicato, ma solo se:

- non duplica dati canonici già esistenti;
- è chiaramente classificato come data di presentazione/design;
- usa Stable Character ID;
- ha validator;
- il collegamento con i Character Definition futuri è documentato.

Roadmap futura:

```text
Unreal Character/Data Assets
        |
        v
deterministic export
        |
        v
Wiki data
        |
        v
SVG generator
```

Non implementare ora il bridge Unreal se fuori scope.

# 9. STRUTTURA FILE PROPOSTA

Adattare ai path reali del repository.

```text
Docs/
  Wiki/
    Data/
      Characters/
        gadget.yaml
        phase.yaml
        riktor.yaml
        wraith.yaml
    Assets/
      Generated/
        radar/
        compare/

Tools/
  WikiChartGen/
    package.json
    tsconfig.json
    src/
      index.ts
      schema.ts
      fsUtils.ts
      renderRadar.ts
      renderCompare.ts
    tests/
```

Non creare queste cartelle se il repo possiede già equivalenti.

# 10. SCHEMA DATI PROPOSTO

Esempio concettuale:

```yaml
id: character.gadget
slug: gadget
displayName: Gadget
role: Controller

profile:
  offense: TBD
  durability: TBD
  mobility: TBD
  control: TBD
  support: TBD
  information: TBD

balance:
  precision: TBD
  power: TBD
  control: TBD
  support: TBD
  durability: TBD

tags:
  - Electric
  - AreaControl
  - ComboStarter
```

Gli esempi numerici discussi in chat non rendono canonico Gadget.

# 11. VALIDAZIONE

Il tool deve almeno validare:

- id presente e univoco;
- slug presente e univoco;
- displayName presente;
- role presente;
- assi previsti;
- nessun asse sconosciuto;
- valori interi 1..10 quando valorizzati;
- comportamento esplicito per TBD/null;
- input duplicati;
- output path deterministico.

Valutare se integrare questi controlli nel sistema validator già esistente del repository oppure tenerli nel tool docs con una Issue futura di convergenza.

# 12. OUTPUT DEL GENERATORE

Single Profile:

```text
<slug>-profile.svg
```

Single Balance:

```text
<slug>-balance.svg
```

Compare:

```text
<slugA>-vs-<slugB>-profile.svg
<slugA>-vs-<slugB>-balance.svg
```

# 13. CONTENUTO VISIVO MINIMO

## Single chart

- nome personaggio;
- ruolo;
- tipo radar;
- griglia;
- assi;
- valori;
- poligono;
- eventualmente tag sintetici.

## Compare chart

- personaggio A;
- personaggio B;
- due poligoni distinguibili;
- legenda;
- stessi assi e stessa scala;
- valori leggibili.

Accessibilità:

- non affidarsi solo al colore;
- usare anche stroke/pattern/legenda;
- SVG con role="img" e descrizione/label appropriata quando possibile.

# 14. CLI PROPOSTA

Adattare al package manager reale del repo.

```bash
pnpm wiki:charts
pnpm wiki:chart gadget
pnpm wiki:compare gadget phase profile
pnpm wiki:compare gadget phase balance
```

Se il repo usa npm/yarn/Make/PowerShell/task runner differente, integrare con quello esistente.

# 15. DETERMINISMO DEL TOOL

A parità di input, versione tool, template e configurazione, l’SVG prodotto deve essere byte-identical o semanticamente deterministico secondo policy esplicita.

Preferenza: **byte-identical output**, evitando timestamp, UUID random, ordine non stabile e metadata variabili.

# 16. TEST MINIMI

### Schema
- file valido;
- field mancante;
- valore <1;
- valore >10;
- float non consentito;
- asse sconosciuto;
- ID duplicato;
- slug duplicato.

### Renderer
- genera SVG valido;
- contiene tutte le label;
- contiene il nome del personaggio;
- compare contiene entrambi i nomi;
- stesso input produce stesso output.

### Golden test
`character fixture -> expected SVG/hash`

### Wiki integration
Verificare che almeno un SVG sia visualizzabile correttamente nella piattaforma Wiki reale usata dal progetto.

# 17. WIKI DA AGGIORNARE

Consolidare nelle pagine reali esistenti.

## Roster Overview
Aggiungere radar Profile per ogni personaggio canonico con valori disponibili, link alla scheda completa, ruolo, tag principali ed eventuali confronti.

## Character Statistics / Character Profiles
Documentare differenza tra stats reali e rating, scala 1–10, sei assi Profile, cinque assi Balance, criterio dei trade-off e status dei valori non definiti.

## Wiki Chart Generator
Documentare source data, comando generazione, output, directory generated, single/compare, validator, golden test e regole per non editare SVG manualmente.

## Character page template
Prevedere, se compatibile:

```text
Portrait / identity
Role
Profile Radar
Core stats
Abilities
Tags
Strengths
Trade-offs
Synergies
Counters
Build variants
Balance Radar (opzionale/internal)
```

# 18. DOCUMENTAZIONE DA AGGIORNARE/CONSOLIDARE

## Character/Ability design
- Character Profile Radar;
- Balance Radar;
- rating non usati dal resolver;
- relazione con varianti/build.

## UI/UX
- radar come componente scheda/roster;
- confronto personaggi;
- leggibilità/accessibilità;
- uso in Wiki e futuro frontend in-game.

## Data/Validation
- schema rating;
- Stable Character ID;
- validator;
- generated artifacts;
- strategia futura Data Asset → Wiki export.

## QA/Roadmap
- generator MVP;
- deterministic test;
- Wiki integration check;
- future CI regeneration.

# 19. ROADMAP — NUOVO EPIC

Creare o aggiornare un Epic equivalente a:

## EPIC — Character Visualization & Wiki Chart Pipeline

**Goal:** fornire una pipeline deterministica e data-driven per generare visualizzazioni comparative dei personaggi e integrarle nella Wiki senza duplicare l’autorità del gameplay.

**Non deve bloccare il core simulator/Fondazioni.**

Collocarlo nella milestone/area più coerente con il repository attuale, probabilmente documentation/tooling, character/balance systems o UI/UX tooling.

# 20. ISSUE BREAKDOWN PROPOSTO

Adatta numerazione, labels e milestone al repository.

## ISSUE 1 — Define Character Radar Rating Model
Formalizzare Profile Radar, Balance Radar, scala 1–10, rubriche e TBD policy.

Acceptance:
- assi approvati;
- significato documentato;
- separazione stats/rating esplicita.

## ISSUE 2 — Define Wiki Character Radar Schema
Schema YAML/JSON o mapping verso formato esistente, stable IDs, validator, TBD/null policy.

## ISSUE 3 — Implement Single Character SVG Radar Generator
Profile, Balance, labels, deterministic rendering, golden test.

## ISSUE 4 — Implement Character Comparison Radar
A vs B, Profile, Balance, accessibilità, stessa scala.

## ISSUE 5 — Add Wiki Chart CLI / Batch Generation
Generate all, single, compare, output path stabile, exit code non-zero su input invalido.

## ISSUE 6 — Integrate Character Radar into Wiki
Roster Overview, pagine personaggio, docs generator.

## ISSUE 7 — Add Generator Automated Tests
Schema, renderer, golden e duplicate tests.

## ISSUE 8 — Define Canonical Ratings for v0.1 Roster
Gadget, Phase, Riktor, Wraith.

Acceptance:
- nessun valore arbitrario;
- valori tracciabili a design/balance;
- rationale registrato;
- trade-off leggibili.

# 21. ISSUE FUTURE / NON MVP

Backlog, senza implementazione automatica se fuori scope:

- Character Card Generator;
- PNG Export;
- Build/Variant Overlay;
- Unreal Data Export Bridge;
- GitHub Action per validate/regenerate/stale artifact check.

# 22. FEATURE MAP

Aggiornare la Feature Map reale con feature equivalenti a:

```text
Character Profile Radar
Character Balance Radar
Character Comparison Radar
Wiki Chart Generator
Character Rating Validation
Wiki Roster Visualization
```

Future:

```text
Character Card Generator
Build Radar Overlay
PNG Export
Unreal-to-Wiki Data Export
Automated Chart Regeneration CI
```

Per ogni feature collegare, se il formato lo consente: status, milestone, Epic/Issue, Wiki/doc link, dependency e acceptance state.

# 23. SCENARIO MAP

Se la Scenario Map supporta QA/tooling:

## DOC-RADAR-01 — Single Character Generation
Input valido → Profile SVG + Balance SVG + output deterministico.

## DOC-RADAR-02 — Character Comparison
Due personaggi validi → compare SVG, stessi assi, legenda corretta.

## DOC-RADAR-03 — Invalid Rating
Valore 11 → validation error, nessun artifact parziale.

## DOC-RADAR-04 — Duplicate Stable ID
Due character con stesso ID → validation error.

## DOC-RADAR-05 — Wiki Render Smoke Test
Generated SVG → rendering leggibile nella Wiki reale.

## DOC-RADAR-06 — Deterministic Regeneration
Stessi dati, due esecuzioni → stesso output/hash.

Se la Scenario Map è gameplay-only, non forzare questi scenari: usare il QA/tooling tracker equivalente e aggiungere solo un riferimento incrociato.

# 24. EDITOR MAP

MVP: **nessun task Unreal Editor obbligatorio.**

Non creare finto lavoro Editor.

Se Editor Map permette N/A/Future/External tooling, registrare:

```text
Wiki Radar Generator
Editor dependency: none
```

Task futuro possibile: export deterministic wiki metadata dai Character Data Assets, solo quando esisterà il bridge.

# 25. ROADMAP / PROGRESS TRACKER

Stato suggerito:

```text
Design decision: Accepted
Spec: Ready
Generator implementation: Planned / Backlog
Wiki integration: Planned
Canonical v0.1 ratings: Needs design/balance pass
Data Asset bridge: Future
CI regeneration: Future
```

Se esiste un Project Control Center basato su YAML, aggiungere record con link a Epic, Issue, Wiki, documentation, generated asset directory e milestone.

# 26. PROJECT CONTROL CENTER

Il progetto usa tracker come Roadmap, Feature Map, Scenario Map ed Editor Map.

Se esiste già il Project Control Center basato su YAML:

1. individuare la source YAML reale;
2. aggiungere le nuove entry senza cambiare schema inutilmente;
3. collegare Epic/Issue/Wiki;
4. verificare che dashboard/pagina continui a caricare i dati.

Non inventare un nuovo tracker separato.

# 27. RELAZIONE CON BUILD E PERSONALIZZAZIONE

Il sistema deve supportare in futuro un radar sovrapposto:

```text
Character Base
      +
Selected Variant / Build
```

Obiettivo: mostrare che una build modifica la forma tattica senza trasformare la progressione in upgrade lineari.

Future:
- Base vs Build;
- Build A vs Build B;
- Character A vs Character B.

MVP: personaggio singolo + confronto personaggi.

# 28. RELAZIONE CON AFFINITY / SYNERGY

Non confondere il radar con Affinity/Synergy.

- Radar: “Qual è il profilo tattico di questo personaggio/build?”
- Affinity: “Verso quale stile tende un oggetto/variante?”
- Interaction: “Le regole dei due elementi funzionano bene insieme?”
- Synergy: “Il sistema riconosce formalmente una relazione.”

In futuro le Affinity possono influenzare il profilo di una build, ma non devono diventare automaticamente nuovi assi del radar.

# 29. RELAZIONE CON OVERWATCH / FAST REACTION / RUMORE

Il Profile Radar deve poter riflettere indirettamente sistemi già consolidati:

- Overwatch/reaction → soprattutto Controllo, Informazione, Supporto o Offesa secondo il kit;
- Rumore/perception → Informazione;
- stealth → Informazione + Mobilità quando appropriato;
- environmental control → Controllo/Supporto;
- cover manipulation → Controllo/Durabilità/Supporto secondo effetto.

Non creare un asse per ogni sistema. Restare a **6 assi leggibili**.

# 30. UX E ACCESSIBILITÀ

Regole:

- massimo 6 assi per Profile;
- 5 assi per Balance;
- non dipendere esclusivamente dal colore;
- label leggibili;
- stessa scala in tutti i confronti;
- ordine assi stabile;
- niente auto-scaling per personaggio;
- valori visibili anche testualmente.

Ordine stabile Profile:

```text
Offesa
Durabilità
Mobilità
Controllo
Supporto
Informazione
```

Ordine stabile Balance:

```text
Precisione
Potenza
Controllo
Supporto
Durabilità
```

L’ordine fa parte della spec perché cambiarlo altera la forma visiva.

# 31. DEFINITION OF DONE DEL GENERATORE MVP

Il generator MVP è Done quando:

1. esiste uno schema validato;
2. genera Profile SVG;
3. genera Balance SVG;
4. genera Compare SVG;
5. stesso input produce stesso output;
6. input invalido fallisce chiaramente;
7. almeno un radar è integrato e visibile nella Wiki;
8. esiste documentazione d’uso;
9. esistono test automatici minimi;
10. Roadmap/Feature Map/Scenario Map o tracker equivalente sono aggiornati;
11. Epic/Issue sono collegate;
12. nessun dato competitivo è duplicato senza ownership documentata.

# 32. NON OBIETTIVI DELL’MVP

Non includere automaticamente:

- animazioni radar;
- editor grafico interattivo;
- chart in-game;
- PNG;
- export diretto Unreal;
- GitHub Action;
- character card completa;
- calcolo automatico perfetto dei rating;
- ML/AI scoring;
- modifica dei valori di gameplay.

# 33. GIT / COMMIT PROPOSTI

Possibile sequenza:

```text
docs(character): define radar profile and balance model
docs(wiki): define generated character visualization pipeline
chore(tracking): add character radar epic and project map entries
feat(wiki-tools): add deterministic svg radar generator
test(wiki-tools): add schema and golden rendering tests
docs(wiki): integrate generated character radar assets
```

Se questa sessione deve fare solo consolidamento/tracking e non implementazione:

```text
docs(character): consolidate radar visualization model
chore(tracking): add wiki chart generator epic and issues
docs(wiki): document planned character radar pipeline
```

# 34. OUTPUT FINALE RICHIESTO A CLAUDE

Al termine rispondi con:

## Consolidato
Decisioni diventate canoniche.

## File modificati
Per ogni file: path, motivo, contenuto principale.

## Wiki
Pagine create/aggiornate.

## Epic
Titolo, milestone, link/ID, scope.

## Issue
Tabella: `ID | Titolo | Milestone | Stato | Dipendenze`.

## Roadmap
Entry create/aggiornate.

## Feature Map
Entry create/aggiornate.

## Scenario Map
Entry create/aggiornate.

## Editor Map
Entry create/aggiornate o dichiarazione “no Editor work for MVP”.

## Project Control Center
File YAML aggiornati e link introdotti.

## Test
Comandi eseguiti e risultato.

## Decisioni aperte
In particolare:
- criteri quantitativi definitivi per kit/stats → rating 1–10;
- rating canonici Gadget/Phase/Riktor/Wraith;
- ownership finale YAML vs export Data Asset;
- policy CI per generated artifacts.

## Next step
Indica una sola attività consigliata immediatamente successiva.

# 35. PRIORITÀ

Ordine raccomandato:

1. consolidare design e ownership dati;
2. aggiornare Wiki/documentazione;
3. aggiornare Roadmap/Feature/Scenario/Editor Map;
4. creare/aggiornare Epic e Issue;
5. solo dopo implementare il generator MVP nella Issue dedicata;
6. definire rating canonici v0.1 con pass di balance separato.

Il radar deve diventare uno strumento utile al progetto, non una nuova fonte di dati incoerente.

# 36. RISULTATO ATTESO

A fine consolidamento il repository deve rendere immediatamente chiaro che:

- RefactorTactics usa radar/spider chart per descrivere i profili dei personaggi;
- esistono due viste distinte: Profile e Balance;
- le statistiche reali sono separate dai rating 1–10;
- i grafici sono artifact generati;
- è prevista una pipeline deterministica per Wiki;
- il lavoro è tracciato da Epic/Issue;
- Roadmap e mappe di progresso sono allineate;
- i valori non ancora approvati restano esplicitamente TBD;
- la futura integrazione con Data Assets è pianificata ma non anticipata nell’MVP.
