> ✅ **RECEPITO il 2026-08-08 da [D-029](../../decisions/RT_PDR_00_Decision_Log.md) /
> [ADR-0006](../../decisions/adr-0006-ownership-abilita-sinergie.md).** Questo handoff resta un **input**, non
> un'autorità (`AGENTS.md`: `docs/src/` non è fonte normativa per default). Gli owner normativi
> dell'ownership dei contenuti sono ora
> [`../gameplay/spec-ownership-abilita-interazioni-sinergie.md`](../../gameplay/spec-ownership-abilita-interazioni-sinergie.md)
> e le pagine Wiki [`../wiki/fazioni/index.md`](../../wiki/fazioni/index.md) ·
> [`../wiki/game/sinergie-e-combinazioni.md`](../../wiki/game/sinergie-e-combinazioni.md).
>
> **Non c'è deriva da correggere qui**: il divieto di `SameFactionDamageBonus` / `SameFactionArmorBonus` /
> `FactionSetBonus` (§3) e la regola «la cooperazione emerge dalle normali meccaniche» sono esattamente ciò che
> D-029 ha consolidato. Il documento **non** propone kit di coppia o di fazione, quindi non è superato: è la
> fonte da cui la decisione è nata. Non riscriverlo.

# REFACTORTACTICS — CONSOLIDAMENTO FAZIONI, IDENTITÀ VISIVA, ROSTER E SCENARI DI COOPERAZIONE

## Prompt operativo per Claude Code

Stai lavorando nella repository **RefactorTactics**.

Questa attività serve a consolidare nel progetto una decisione di design relativa a:

- fazioni;
- appartenenza dei personaggi;
- identità narrativa e tattica;
- palette colori;
- separazione tra colore personaggio, colore fazione e colore squadra;
- Wiki delle fazioni;
- Wiki dei personaggi;
- dati strutturati/cataloghi;
- scenari automatici di cooperazione tra personaggi della stessa fazione;
- riferimenti dalla Wiki agli scenari eseguibili;
- roadmap, test e validazione dei dati.

Non limitarti ad aggiungere un documento isolato: integra la decisione nelle fonti di verità esistenti della repository.

---

# 0. MODALITÀ DI LAVORO

Prima di modificare qualsiasi file:

1. analizza la repository;
2. leggi `AGENTS.md`, `CLAUDE.md` e altre istruzioni locali se presenti;
3. individua:
   - documentazione gameplay;
   - documentazione lore/worldbuilding;
   - Wiki personaggi;
   - eventuale Wiki fazioni;
   - roster/cataloghi personaggi;
   - Data Assets / JSON / CSV / XLSX o altre fonti dati;
   - scenario/test harness;
   - scenari automatici esistenti;
   - roadmap;
   - Decision Log / ADR;
   - validator;
   - eventuali enum/tag/ID di fazione già implementati;
4. identifica la fonte di verità per:
   - `CharacterId`;
   - `FactionId`;
   - roster per versione;
   - palette/UI;
   - scenari;
5. cerca riferimenti obsoleti alle fazioni o al roster;
6. segnala conflitti prima di sostituire dati canonici recenti;
7. usa la versione Unreal Engine realmente bloccata nel repository;
8. non inventare API Unreal.

Regola di prevalenza:

```text
Decisioni esplicite più recenti
    >
Decision Log / ADR aggiornati
    >
dati canonici correnti
    >
documentazione storica
    >
proposte precedenti
```

Se trovi vecchie proposte incompatibili con questa specifica, aggiornale o archiviale secondo le convenzioni della repository, mantenendo provenance quando utile.

---

# 1. DECISIONE CANONICA — QUATTRO FAZIONI

Il roster iniziale di RefactorTactics deve essere organizzato in **quattro fazioni**.

Le fazioni NON sono classi e NON determinano automaticamente il ruolo del personaggio.

Le fazioni esprimono:

- identità narrativa;
- filosofia;
- dottrina tattica;
- linguaggio visivo;
- legami tra personaggi;
- esempi naturali di cooperazione.

Non esiste alcun bonus implicito per una squadra composta da membri della stessa fazione.

## Roster canonico

### v0.1

| Character | Faction |
|---|---|
| Flux | Conflux |
| Riva | Conflux |
| Bastion | Constrine |
| Vektor | Constrine |

### v0.2

| Character | Faction |
|---|---|
| Steel | Sentinel Directorate |
| Murdock | Sentinel Directorate |
| Aurora | Resonance |
| Kwang | Resonance |

La v0.2 aggiunge quindi due nuove fazioni.

Non usare vecchi personaggi/proposte sostitutive del roster se sono già stati dichiarati obsoleti.

Roster operativo attuale:

```text
Flux
Riva
Bastion
Vektor
Steel
Aurora
Murdock
Kwang
```

---

# 2. ID STABILI DELLE FAZIONI

Usare ID stabili coerenti con le convenzioni del progetto.

Baseline proposta:

```text
Faction.Conflux
Faction.Constrine
Faction.Sentinel
Faction.Resonance
```

Display name:

```text
Conflux
Constrine
Sentinel Directorate
Resonance
```

IMPORTANTE:

- verificare prima se esiste già uno schema `FactionId`;
- non introdurre un secondo formato parallelo;
- il display name può cambiare/localizzarsi;
- lo Stable ID non deve dipendere dal testo localizzato.

Se il progetto usa Gameplay Tags per le fazioni, integrare nel dizionario governato esistente.

Non creare tag liberamente negli asset se il progetto usa una whitelist/tag source governata.

---

# 3. PRINCIPIO DI GAME DESIGN

## Fazione != classe

Un personaggio appartenente a una fazione può svolgere ruoli differenti.

Esempio:

```text
Sentinel Directorate
    Steel   -> controllo ravvicinato / protezione
    Murdock -> controllo a distanza / Fire Sector
```

## Fazione != composizione obbligatoria

Una squadra può contenere liberamente personaggi appartenenti a fazioni diverse.

Esempio valido:

```text
Flux
Steel
Aurora
```

Non introdurre:

```text
SameFactionDamageBonus
SameFactionArmorBonus
FactionSetBonus
```

o equivalenti senza una futura decisione esplicita.

Le sinergie fra membri della stessa fazione devono emergere dalle normali meccaniche del gioco.

---

# 4. CONFLUX

## Stable ID

```text
Faction.Conflux
```

## Membri iniziali

```text
Flux
Riva
```

## Identità

Principio:

> Tutto è collegato.

Conflux interpreta campo di battaglia, energia, ambiente e combattenti come parti di uno stesso sistema dinamico.

La fazione non cerca di congelare il campo in una configurazione ideale.

Preferisce:

```text
trasformare
    ->
collegare
    ->
propagare
    ->
sfruttare la conseguenza
```

## Domanda tattica

> Come posso collegare e trasformare ciò che esiste sulla mappa?

## Motto provvisorio

```text
Nothing stands alone.
Nulla esiste isolato.
```

## Linguaggio visivo

- curve;
- connessioni;
- nodi;
- flussi;
- circuiti organici;
- geometria aperta;
- transizioni.

## Simbolo concettuale

Tre nodi connessi da linee curve che convergono senza creare una struttura completamente chiusa.

Non generare necessariamente il logo definitivo in questa task se non esiste una pipeline art dedicata.

## Palette fazione

| Token | HEX |
|---|---|
| Primary / Conflux Teal | `#16C7B7` |
| Secondary / Flow Cyan | `#39BDF2` |
| Dark / Deep Current | `#10272D` |
| Light / Mist | `#DDF7F3` |

## Personaggi

### Flux

| Token | HEX |
|---|---|
| Primary | `#12CDE3` |
| Secondary | `#25313B` |
| Accent | `#8C6CFF` |
| Faction Mark | `#16C7B7` |

Identità tattica:

```text
Conductive Network
connection
electric propagation
nodes
environmental exploitation
```

### Riva

| Token | HEX |
|---|---|
| Primary | `#36BFC5` |
| Secondary | `#174B64` |
| Accent | `#DDF7F3` |
| Faction Mark | `#16C7B7` |

Identità tattica:

```text
Wet Territory
flow
water
displacement
terrain transformation
```

---

# 5. CONSTRINE

## Stable ID

```text
Faction.Constrine
```

## Membri iniziali

```text
Bastion
Vektor
```

## Identità

Principio:

> Ciò che è delimitato può essere controllato.

Constrine considera il caos un problema di possibilità e geometria.

La dottrina consiste nel:

```text
ridurre le opzioni
    ->
canalizzare
    ->
rendere prevedibile
    ->
punire la scelta rimasta
```

## Domanda tattica

> Come restringo le possibilità finché il nemico diventa prevedibile?

## Motto provvisorio

```text
Define the possible.
Definisci ciò che è possibile.
```

## Linguaggio visivo

- angoli netti;
- segmenti;
- parentesi;
- strutture;
- reticoli;
- geometria chiusa;
- delimitazioni.

## Simbolo concettuale

Esagono incompleto delimitato da due strutture laterali.

## Palette fazione

| Token | HEX |
|---|---|
| Primary / Constrine Gold | `#D7A83E` |
| Secondary / Structural Slate | `#67717D` |
| Dark / Charcoal | `#20242A` |
| Light / Ivory | `#E9E2D3` |

## Personaggi

### Bastion

| Token | HEX |
|---|---|
| Primary | `#B97842` |
| Secondary | `#343A42` |
| Accent | `#F0B94C` |
| Faction Mark | `#D7A83E` |

Identità tattica:

```text
Directional Structures
cover
edge manipulation
route control
defensive geometry
```

### Vektor

| Token | HEX |
|---|---|
| Primary | `#8D3851` |
| Secondary | `#303641` |
| Accent | `#D8B766` |
| Faction Mark | `#D7A83E` |

Identità tattica:

```text
Prediction
Interception
route reading
delayed/predictive pressure
```

---

# 6. SENTINEL DIRECTORATE

## Stable ID

```text
Faction.Sentinel
```

## Display Name

```text
Sentinel Directorate
```

## Membri iniziali

```text
Steel
Murdock
```

## Identità

Principio:

> Una posizione controllata diventa una certezza.

Sentinel Directorate è orientata a:

- protezione;
- sicurezza;
- controllo territoriale;
- superiorità locale;
- sorveglianza;
- risposta preparata.

NON è equivalente a Constrine.

Differenza semantica fondamentale:

```text
Constrine:
"Ti costringo a passare da lì."

Sentinel:
"Se passi da qui, siamo pronti."
```

## Domanda tattica

> Quale zona dobbiamo rendere imprendibile?

## Motto provvisorio

```text
Hold what matters.
Mantieni ciò che conta.
```

## Linguaggio visivo

- piastre;
- scudi;
- settori;
- reticoli radar;
- indicatori di sorveglianza;
- geometria robusta;
- layering protettivo.

## Simbolo concettuale

Scudo stilizzato attraversato da un settore di tiro/radar.

## Palette fazione

| Token | HEX |
|---|---|
| Primary / Sentinel Cobalt | `#356FF6` |
| Secondary / Steel | `#8794A4` |
| Dark / Command Navy | `#172538` |
| Light / Armor White | `#E3EBF4` |

## Personaggi

### Steel

| Token | HEX |
|---|---|
| Primary | `#547693` |
| Secondary | `#343C45` |
| Accent | `#D9E5F0` |
| Faction Mark | `#356FF6` |

Identità tattica:

```text
Guard Meter
protection
interposition
close-area denial
body control
reaction
```

### Murdock

| Token | HEX |
|---|---|
| Primary | `#192B45` |
| Secondary | `#59697D` |
| Accent | `#497FF5` |
| Optic Micro Accent | `#D95353` |
| Faction Mark | `#356FF6` |

Identità tattica:

```text
Focus
Fire Sector
Overwatch
ranged area control
long-range surveillance
```

---

# 7. RESONANCE

## Stable ID

```text
Faction.Resonance
```

## Membri iniziali

```text
Aurora
Kwang
```

## Identità

Principio:

> Potere e posizione hanno valore quando entrano in risonanza.

Resonance interpreta il combattimento come una sequenza:

```text
preparazione
    ->
allineamento
    ->
momento favorevole
    ->
esecuzione
```

Non deve diventare semplicemente la "fazione degli elementi".

L'elemento è uno strumento.

La fazione descrive **come viene usato**.

## Domanda tattica

> Come preparo il campo e il momento giusto per entrare in sintonia?

## Motto provvisorio

```text
Find the moment.
Trova il momento.
```

## Linguaggio visivo

- cerchi concentrici;
- onde;
- geometria radiale;
- ancore;
- nuclei;
- pattern ritmici;
- strutture concentriche.

## Simbolo concettuale

Due anelli interrotti attorno a un nucleo centrale.

## Palette fazione

| Token | HEX |
|---|---|
| Primary / Resonance Indigo | `#725CF4` |
| Secondary / Resonant Lavender | `#B8A9F7` |
| Dark / Obsidian Violet | `#292638` |
| Light / Silver | `#DDDDEC` |

## Personaggi

### Aurora

| Token | HEX |
|---|---|
| Primary | `#9CE5F5` |
| Secondary | `#EEF8FA` |
| Accent | `#A38BEF` |
| Faction Mark | `#725CF4` |

Identità tattica:

```text
Frozen Domain
terrain shaping
slow
route alteration
controlled preparation
```

### Kwang

| Token | HEX |
|---|---|
| Primary | `#6547D8` |
| Secondary | `#292736` |
| Accent | `#E5B84B` |
| Faction Mark | `#725CF4` |

Identità tattica:

```text
Electric Anchor
anchor-based positioning
pressure point
commitment
energy geometry
```

---

# 8. REGOLA VISIVA — PERSONAGGIO, FAZIONE E SQUADRA SONO TRE LIVELLI DISTINTI

Questa distinzione è obbligatoria.

## 8.1 Colore personaggio

Definisce l'identità visiva individuale.

Deve rimanere riconoscibile indipendentemente dalla composizione della squadra.

## 8.2 Colore fazione

Deve comparire come elemento identitario secondario:

- insignia;
- trim;
- badge;
- dettaglio UI;
- piccola marcatura costume;
- bordatura della scheda Wiki;
- eventuale pattern.

NON deve sostituire la palette principale del personaggio.

## 8.3 Colore squadra

È runtime e dipende dal match.

Non recolorare completamente il personaggio.

Usare preferibilmente:

- base ring;
- outline;
- marker;
- path;
- destination;
- healthbar underline;
- selection;
- AoE outline;
- team icon;
- minimap/tactical indicators.

Baseline proposta:

| Match affiliation | HEX |
|---|---|
| Friendly | `#45D69B` |
| Enemy | `#FF6678` |

Verificare eventuali palette UI già esistenti prima di applicarle.

La leggibilità NON deve affidarsi solo al colore.

Affiancare:

- forme;
- icone;
- pattern;
- testo;
- indicatori direzionali.

Rispettare le regole di accessibilità già documentate.

---

# 9. MODELLO DATI DA CONSOLIDARE

Verificare come sono attualmente memorizzati i personaggi e non creare fonti parallele.

Ogni Character Definition dovrebbe poter derivare almeno:

```text
CharacterId
DisplayName
FactionId
ReleaseVersion
Role
SignatureMechanic
VisualPalette
FactionMarkColor
```

NON aggiungere campi duplicati se già esistono strutture equivalenti.

La palette può essere:

- strutturata nel Character Data;
- referenziata tramite un Visual Identity asset;
- gestita da un catalogo visuale separato;

scegliere la soluzione coerente con il progetto attuale.

## Faction Definition

Se non esiste già, valutare una definizione data-driven tipo:

```text
FactionId
Version
DisplayName
ShortDescription
Motto
Doctrine
Values
TacticalIdentity
VisualLanguage
PrimaryColor
SecondaryColor
DarkColor
LightColor
MemberCharacterIds
WikiPage
FeaturedScenarioIds
```

NON implementare un `UPrimaryDataAsset` nuovo solo perché proposto qui se il repository utilizza già un altro modello.

Prima individuare l'architettura corrente.

---

# 10. WIKI DELLE FAZIONI

Creare o aggiornare una pagina Wiki per ciascuna fazione.

Pagine richieste:

```text
Conflux
Constrine
Sentinel Directorate
Resonance
```

Usare percorso e naming coerenti con la Wiki esistente.

## Schema minimo

```text
# Nome fazione

FactionId
Version / first appearance

## Sintesi

## Motto

## Identità

## Storia / Lore
(se non ancora definita completamente, distinguere chiaramente CANON da TBD)

## Filosofia

## Valori

## Dottrina tattica

## Linguaggio visivo

## Simbolo

## Palette

## Membri

## Cooperazioni note

## Scenari giocabili

## Collegamenti
- personaggi
- meccaniche
- scenari
```

## Lore

NON inventare retroscena storici dettagliati senza supporto documentale.

Le identità filosofiche/tattiche definite in questo documento sono canoniche.

Eventuali dettagli come:

- origine geopolitica;
- leadership;
- pianeta/territorio;
- guerre;
- date;
- fondatori;

devono rimanere `TBD` se non già canonici.

---

# 11. WIKI PERSONAGGI

Aggiornare le pagine degli otto personaggi.

Aggiungere almeno:

```text
Faction: <link>
FactionId: ...
Faction Mark / palette
Release Version
Signature Mechanic
Related Faction Scenarios
```

Ogni personaggio deve collegarsi:

```text
Character Wiki
    ->
Faction Wiki
    ->
Scenario Wiki / Scenario ID
```

e viceversa.

---

# 12. SCENARI DI COOPERAZIONE DELLE FAZIONI

Gli scenari devono essere scenari reali eseguibili dal Test Harness del progetto.

NON creare semplici esempi narrativi scollegati dal sistema automatico.

Devono usare:

```text
Scenario
    ->
normal intents/commands
    ->
Planning
    ->
Commit
    ->
Snapshot
    ->
Resolver
    ->
TurnLog
```

Non creare scorciatoie specifiche per la showcase.

## Cartella proposta

Adattare al layout reale:

```text
Tests/Scenarios/Factions/
    Conflux/
    Constrine/
    Sentinel/
    Resonance/
```

Se esiste già una directory canonica per gli scenari, usare quella.

---

# 13. SCENARIO CONFLUX

## Stable Scenario ID

```text
Team.Conflux.FluxRiva.ConductiveFlood
```

## Titolo

```text
Conductive Flood
```

## Personaggi

```text
Flux
Riva
```

## Obiettivo

Dimostrare cooperazione sistemica:

```text
Riva modifica terreno
    ->
Wet Territory
    ->
il posizionamento/percorso cambia
    ->
Flux crea connessione conduttiva
    ->
payoff elettrico
```

## Feature da mostrare

- acqua;
- Wet;
- displacement o controllo percorso;
- conductive network;
- elettricità;
- propagazione;
- TurnLog ambiente;
- sinergia emergente, non faction bonus.

## Sequenza showcase indicativa

Turno 1:
- Riva crea/estende acqua verso un choke point.
- Flux prepara un nodo o una connessione.

Turno 2:
- Riva modifica il posizionamento o incentiva una rotta attraverso area Wet.
- Flux completa la geometria conduttiva.

Turno 3:
- Flux usa la rete creata per il payoff elettrico.

Adattare questa sequenza alle abilità effettivamente implementate.

NON inventare AbilityId che non esistono.

---

# 14. SCENARIO CONSTRINE

## Stable Scenario ID

```text
Team.Constrine.BastionVektor.OnlyExit
```

## Titolo

```text
The Only Exit
```

## Personaggi

```text
Bastion
Vektor
```

## Obiettivo

Dimostrare:

```text
Bastion restringe le rotte
    ->
una soluzione diventa più probabile
    ->
Vektor predice/intercetta
```

## Feature

- cover direzionale;
- edge/route control;
- path shaping;
- prediction;
- interception;
- delayed/predictive action se disponibile;
- counterplay;
- explainability nel TurnLog.

Adattare lo scenario allo stato reale delle meccaniche.

Se una feature non è ancora implementata:

- non simularla con hack;
- segnalarla;
- creare scenario gated/TODO coerente con la roadmap.

---

# 15. SCENARIO SENTINEL DIRECTORATE

## Stable Scenario ID

```text
Team.Sentinel.SteelMurdock.HoldTheLine
```

## Titolo

```text
Hold the Line
```

## Personaggi

```text
Steel
Murdock
```

## Obiettivo

Mostrare due strati complementari di controllo:

```text
Steel   -> spazio vicino
Murdock -> spazio lontano
```

## Setup concettuale

I due personaggi difendono:

```text
Relay / Objective / Choke Point
```

## Feature

- protezione;
- interposizione;
- reaction;
- Fire Sector;
- facing;
- Overwatch;
- controllo area;
- objective defense;
- Reaction Opportunity;
- TurnLog delle reaction.

## Sequenza indicativa

1. Murdock definisce un settore di tiro.
2. Steel occupa una rotta che permetterebbe di aggirare il settore.
3. Un nemico pressa Murdock o l'obiettivo.
4. Steel produce una protezione/interposizione realmente significativa.
5. Un altro nemico entra nel Fire Sector.
6. Murdock riceve e risolve una Reaction Opportunity.

Usare le abilità reali e le policy di reaction del progetto.

---

# 16. SCENARIO RESONANCE

## Stable Scenario ID

```text
Team.Resonance.AuroraKwang.FrozenAnchor
```

## Titolo

```text
Frozen Anchor
```

## Personaggi

```text
Aurora
Kwang
```

## Obiettivo

Dimostrare:

```text
Aurora prepara la geometria
    ->
le rotte cambiano valore
    ->
Kwang crea Electric Anchor nel punto diventato strategico
    ->
pressione / reposition / payoff
```

## Principio importante

NON ridurre la cooperazione a:

```text
Ice + Electricity = Damage Bonus
```

La cooperazione deve essere principalmente:

```text
terrain shaping
+
positional anchor
```

## Feature

- Frozen Domain;
- terrain shaping;
- route cost/control;
- Electric Anchor;
- positioning;
- pressure point;
- timing;
- eventuali reaction/commit se realmente presenti nel kit.

---

# 17. RIFERIMENTI SCENARIO DALLA WIKI

Ogni pagina fazione deve avere una sezione **Scenari giocabili**.

Esempio:

```markdown
## Scenari giocabili

### Conductive Flood

- ScenarioId: `Team.Conflux.FluxRiva.ConductiveFlood`
- Characters: Flux + Riva
- Purpose: Wet Territory -> Conductive Network -> Electric payoff
- Mode consigliata: Visual
- Scenario file: `<path reale>`
- Launch preset: `<se supportato>`
```

L'obiettivo è permettere a un developer/designer di leggere la Wiki e identificare immediatamente lo scenario da lanciare.

Se il progetto possiede un Scenario Registry/catalogo, fare in modo che la Wiki utilizzi gli stessi Stable ID.

---

# 18. LAUNCH / EDITOR EXPERIENCE

Integrare gli scenari nel workflow già previsto per gli automated scenario test.

Obiettivo UX:

```text
Open L_DevSandbox
    ->
select BP_GameMode / RTTestDirector / equivalente reale
    ->
Scenario category: Factions
    ->
Faction: Conflux / Constrine / Sentinel / Resonance
    ->
Scenario
    ->
Play
```

IMPORTANTE:

la configurazione effettiva deve rispettare l'architettura già presente.

Se il progetto ha recentemente deciso di classificare gli scenari nel `BP_GameMode`, integrare la categoria **Factions** nel sistema esistente invece di creare un selector parallelo.

Valutare sottocategorie:

```text
Faction
    Conflux
    Constrine
    Sentinel
    Resonance
```

oppure metadata:

```text
Category = Factions
Subcategory = Conflux
```

Scegliere il modello più semplice compatibile con il selector attuale.

---

# 19. SCENARIO METADATA

Ogni scenario dovrebbe essere interrogabile almeno per:

```text
ScenarioId
Version
DisplayName
Category
FactionId
CharacterIds
MapId
Purpose
DemonstratedFeatures
RequiredCapabilities
ExpectedDuration
RecommendedMode
Status
```

Possibili Status:

```text
Active
Gated
Draft
Deprecated
```

Non implementare una nuova infrastruttura se queste informazioni esistono già.

---

# 20. VALIDATOR

Aggiornare/estendere il validator dei contenuti.

Controlli richiesti dove compatibili con l'architettura:

## Fazioni

- FactionId non vuoto;
- FactionId unico;
- Character->Faction valido;
- membro referenziato esistente;
- colore valido;
- Wiki link valido se il sistema li indicizza;
- nessun membro duplicato nella stessa lista.

## Personaggi

- FactionId risolvibile;
- release version valida;
- palette presente;
- Signature Mechanic presente se richiesta dal catalogo.

## Scenari

- ScenarioId unico;
- FactionId valido;
- CharacterIds validi;
- i personaggi dichiarati appartengono alla fazione dello scenario quando lo scenario è marcato `SameFactionCooperation`;
- MapId valido;
- ability/character references valide;
- scenario launchable quando `Status=Active`;
- Wiki reference risolvibile se automatizzabile.

Non bloccare la build per campi lore puramente editoriali salvo che il sistema corrente lo richieda.

---

# 21. TEST

Aggiungere test adeguati al livello realmente implementato.

Minimo:

## Data tests

1. tutti gli 8 personaggi risolvono il FactionId corretto;
2. i 4 FactionId sono unici;
3. nessuna Character Definition ha FactionId invalido;
4. ogni FeaturedScenarioId della fazione esiste.

## Scenario validation

1. ogni scenario è caricabile;
2. CharacterIds risolvono;
3. MapId risolve;
4. le abilità referenziate esistono;
5. lo scenario produce un report machine-readable.

## Gameplay scenario test

Quando le meccaniche necessarie esistono:

```text
Team.Conflux.FluxRiva.ConductiveFlood
Team.Constrine.BastionVektor.OnlyExit
Team.Sentinel.SteelMurdock.HoldTheLine
Team.Resonance.AuroraKwang.FrozenAnchor
```

Devono poter essere eseguiti:

- Visual;
- Fast;
- Headless, quando supportato.

## Determinismo

Per gli scenari Active:

```text
same scenario
same state
same intents
same rules
same seed
    ->
same StateHash
same LogHash
```

---

# 22. DOCUMENTAZIONE DA AGGIORNARE

Cerca e aggiorna almeno le aree reali equivalenti a:

- roster;
- character design;
- factions/lore;
- gameplay overview;
- vertical slice v0.1;
- v0.2;
- scenario catalog;
- test harness docs;
- UI/UX visual identity;
- data model;
- roadmap;
- testing strategy;
- Decision Log;
- changelog.

Non cambiare documenti storici marcati archive se non serve; eventualmente aggiungere nota di superseded.

---

# 23. DECISION LOG

Aggiungere una decisione esplicita equivalente a:

```text
DECISION — Four-faction initial roster

The initial eight-character roster is distributed across four factions:

Conflux:
- Flux
- Riva

Constrine:
- Bastion
- Vektor

Sentinel Directorate:
- Steel
- Murdock

Resonance:
- Aurora
- Kwang

Faction identity is narrative/tactical/visual.
Faction membership does not grant automatic gameplay bonuses.
Match team affiliation is visually distinct from faction and character identity.
Each faction owns at least one launchable same-faction cooperation scenario referenced by its Wiki page.
```

Usare formato e ID reali del Decision Log.

---

# 24. ROADMAP

Integrare il lavoro nella roadmap esistente, senza creare milestone artificiali.

Possibili task:

```text
Faction data model
Faction wiki
Character faction links
Visual palette tokens
Scenario metadata
Faction scenario category
4 cooperation scenarios
Wiki -> scenario references
Validators
Automated scenario tests
```

Per feature non implementabili immediatamente, creare/generare issue o backlog item secondo il workflow reale della repository.

Non dichiarare Done uno scenario se la meccanica che deve dimostrare non è ancora disponibile.

---

# 25. UI / EDITOR

Se esiste un selector di scenari nel GameMode/Blueprint:

aggiungere la tipologia:

```text
Factions
```

con filtraggio per fazione.

Esempio UX:

```text
Scenario Category
    Debug
    Actions
    Factions
    Characters
    Environment
    Reactions
    ...
```

Dentro `Factions`:

```text
Conflux
Constrine
Sentinel Directorate
Resonance
```

Non hardcodare una seconda lista in Blueprint se il catalogo scenario può fornire automaticamente i metadata.

Preferire data-driven.

---

# 26. COLOR TOKENS

Evitare di spargere HEX arbitrari tra Blueprint e widget.

Se il progetto ha un sistema di Style/Data Assets, consolidare token equivalenti a:

```text
Faction.Conflux.Primary
Faction.Conflux.Secondary
Faction.Conflux.Dark
Faction.Conflux.Light

Faction.Constrine.Primary
...

Faction.Sentinel.Primary
...

Faction.Resonance.Primary
...
```

e token personaggio equivalenti.

Team color deve rimanere separato.

Esempio:

```text
Team.Friendly
Team.Enemy
```

Non collegare:

```text
Team.Friendly = Faction.Conflux
```

---

# 27. NON OBIETTIVI

Questa attività NON deve introdurre:

- faction bonus;
- progressione di fazione;
- reputazione;
- matchmaking per fazione;
- lock dei personaggi per fazione;
- guerra geopolitica completa;
- shop;
- battle pass;
- modding pubblico;
- 3D logo production;
- cinematic;
- nuova simulazione per gli scenari;
- AI speciale per le fazioni.

---

# 28. ACCEPTANCE CRITERIA

La task è completata quando:

1. esistono quattro FactionId canonici;
2. tutti gli otto personaggi hanno la fazione corretta;
3. non rimangono `Faction=TBD` per Steel, Murdock, Aurora, Kwang;
4. Conflux/Constrine non sono più considerate le uniche due fazioni dell'intero roster iniziale;
5. esistono quattro pagine Wiki fazione;
6. ogni pagina collega i propri membri;
7. ogni pagina contiene almeno un `ScenarioId` giocabile o correttamente `Gated`;
8. le pagine dei personaggi collegano la fazione;
9. palette personaggio e palette fazione sono definite;
10. team colors sono separati dai faction colors;
11. esistono i quattro Stable Scenario ID;
12. gli scenari realmente supportati sono caricabili dal Test Harness;
13. gli scenari sono classificabili come `Factions`;
14. i dati passano i validator;
15. i test automatici pertinenti passano;
16. roadmap/Decision Log/changelog sono aggiornati;
17. non sono stati introdotti bonus di gameplay impliciti basati sulla fazione;
18. non sono stati duplicati cataloghi/fonti di verità;
19. eventuali gap sono elencati chiaramente;
20. la repository resta compilabile e i test esistenti non regrediscono.

---

# 29. OUTPUT RICHIESTO A CLAUDE

Al termine, produrre un report con:

## A. Audit iniziale

```text
File analizzati
Fonti di verità individuate
Conflitti trovati
Decisioni obsolete trovate
```

## B. Modifiche effettuate

Per ogni file:

```text
path
tipo modifica
motivazione
```

## C. Data model

Mostrare:

```text
Faction IDs
Character -> Faction mapping
Scenario IDs
Palette tokens
```

## D. Wiki

Elenco pagine create/aggiornate e link interni.

## E. Scenari

Per ognuno:

```text
ScenarioId
Status
Path
Map
Characters
Features
How to launch
Expected result
```

## F. Test

```text
test eseguiti
PASS/FAIL
```

## G. Gap

Separare:

```text
BLOCKING
NON-BLOCKING
FUTURE
```

## H. Git

Proporre commit focalizzati.

---

# 30. COMMIT SEQUENCE PROPOSTA

Adattare allo stato reale del repository.

```text
docs(factions): define four-faction roster and visual identities

data(factions): add faction metadata and character mappings

docs(wiki): add faction pages and cross-link character pages

test(scenarios): add faction cooperation scenario metadata

feat(editor): expose faction scenario category

test(data): validate faction and scenario references

docs(roadmap): integrate faction showcase and scenario work
```

Evitare un mega-commit se il repository consente commit separati e verificabili.

---

# 31. RISULTATO DESIDERATO

La repository deve poter rispondere in modo univoco alle domande:

```text
A quale fazione appartiene Steel?
-> Sentinel Directorate

Qual è il colore identitario della fazione?
-> Sentinel Cobalt #356FF6

Qual è il colore della squadra di Steel in una partita?
-> dipende dalla squadra runtime, non dalla fazione

Quale scenario mostra la cooperazione Steel + Murdock?
-> Team.Sentinel.SteelMurdock.HoldTheLine

Dove lo trovo?
-> dalla Wiki Sentinel + catalogo Scenario

Posso lanciarlo?
-> sì, se Status=Active; altrimenti la Wiki indica chiaramente Gated e il requisito mancante
```

Obiettivo finale:

```text
LORE
  |
FACTION DATA
  |
CHARACTER DATA
  |
WIKI
  |
SCENARIO CATALOG
  |
TEST HARNESS
  |
TURN LOG / REPORT
```

Tutto deve usare gli stessi Stable ID e le stesse fonti di verità.

Non creare isole documentali.
