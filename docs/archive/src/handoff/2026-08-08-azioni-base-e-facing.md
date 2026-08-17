# REFACTORTACTICS — HANDOFF PER CLAUDE CODE
> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

## Consolidamento azioni base, Dash, Facing, Pivot e relative conseguenze su docs / roadmap / wiki / feature registry / test

Data: 2026-08-08  
Target: repository principale **RefactorTactics** + wiki separata **`refactor-tactics-main.wiki`**  
Scopo: **consolidamento progettuale e documentale**, non implementazione indiscriminata di gameplay.

---

# 0. MISSIONE

Devi consolidare nel progetto RefactorTactics le decisioni e le direzioni di design emerse sulla **personalizzazione delle azioni base per personaggio** e sul **Facing come stato tattico di prima classe**.

Il lavoro deve aggiornare, senza creare fonti concorrenti:

1. documentazione tecnica e gameplay attiva;
2. Decision Log / ADR, solo se la repository usa questi strumenti per decisioni di questo livello;
3. roadmap reale del progetto;
4. feature registry / lista canonica delle feature **nel posto dove il progetto la salva realmente**;
5. wiki GitHub separata `refactor-tactics-main.wiki`;
6. scenari di validazione;
7. lista dei test PIE / Functional / Automation / golden / network necessari;
8. eventuali matrici XLSX che sono fonte dati per wiki, roster, bilanciamento o feature registry;
9. issue / epic GitHub già esistenti, creando nuove issue solo se non esiste già un contenitore corretto.

NON devi creare una seconda documentazione parallela.
NON devi inventare percorsi.
NON devi creare un nuovo `FEATURE_REGISTRY.md` solo perché questo prompt usa il termine “feature registry”.

Prima devi identificare dove il progetto salva davvero queste informazioni.

---

# 1. AUDIT OBBLIGATORIO PRIMA DELLE MODIFICHE

Prima di modificare qualsiasi file:

1. leggi `AGENTS.md`, `CLAUDE.md` e istruzioni equivalenti presenti nella repository;
2. individua la versione Unreal realmente bloccata dal progetto;
3. individua la roadmap canonica reale;
4. individua il Feature Registry / catalogo feature canonico reale;
5. cerca riferimenti a:
   - `FEATURE_REGISTRY`;
   - `FeatureRegistry`;
   - `feature registry`;
   - roadmap;
   - feature IDs / checkpoint / epic;
6. considera che può esistere un handoff chiamato circa:
   `RefactorTactics_FeatureRegistry_Roadmap_Wiki_Claude_2026-08-08.md`.
   Questo NON va automaticamente considerato il registry: verifica la fonte canonica;
7. individua la struttura degli scenari di test già usata;
8. individua dove vengono registrati i test PIE / Automation / Functional;
9. individua eventuali `.xlsx` che alimentano wiki, cataloghi o matrici di bilanciamento;
10. apri anche la repository wiki separata `refactor-tactics-main.wiki` e individua le pagine effettivamente coinvolte.

Se un percorso o un file non esiste, NON inventarlo: trova l'equivalente già usato dal progetto oppure segnala il gap.

---

# 2. REGOLE DI PREVALENZA

Quando trovi conflitti:

1. decisioni esplicite più recenti del progetto;
2. Decision Log / ADR attivi;
3. stato corrente del codice e dei dati;
4. documentazione gameplay attiva;
5. roadmap e feature registry correnti;
6. vecchi PDR / materiale storico;
7. vecchi handoff AI.

NON cancellare la storia: se un documento è superato, archiviarlo o marcarlo come historical/deprecated secondo le convenzioni esistenti.

NON “correggere silenziosamente” conflitti importanti: riportali nel report finale.

---

# 3. CANONE DI TURNO DA PRESERVARE

Il ciclo fondamentale resta:

```text
PLANNING / DECISION
        ↓
       PREP
        ↓
       DASH
        ↓
       BLAST
        ↓
       MOVE
        ↓
      CLEANUP
```

Vincoli:

- il normale `Move` è sempre l'ultima azione volontaria/fase di movimento del turno;
- non reintrodurre sequenze arbitrarie tipo `Move -> Attack`;
- Dash e altri spostamenti speciali possono avvenire prima perché appartengono alla fase DASH;
- Fast Action / Fast Reaction sono **Decision Boundary**, non una nuova fase principale;
- reaction e finestre live non devono alterare il determinismo del resolver.

---

# 4. LINGUAGGIO DELLE AZIONI BASE

Direzione di design da consolidare:

> Le azioni base sono universali come linguaggio del gioco, ma il modo in cui vengono eseguite può essere fortemente caratterizzato dal personaggio.

Il giocatore deve imparare una volta il significato delle categorie, ma ogni eroe deve “sentirsi” diverso anche senza usare una delle sue quattro abilità speciali.

Famiglia player-facing da considerare:

```text
Move
Dash
Basic Attack
Brace / Guard
Overwatch
Interact
Wait
```

ATTENZIONE: verifica la tassonomia tecnica corrente.
La wiki può presentare Dash come azione base universalmente comprensibile, mentre tecnicamente il codice/catalogo può classificare Dash come **special pre-Blast movement**.
Non appiattire questa distinzione se il progetto la usa già.

## 4.1 Gerarchia di variazione

Modello concettuale:

```text
Universal Base Action
        ↓
Role / Archetype tendencies
        ↓
Character Action Profile
        ↓
Build / Specialization / Talent / Gadget variants
        ↓
Context: terrain / status / temporary effects
```

Regole:

- il ruolo suggerisce una tendenza, NON deve imporre automaticamente valori identici a tutti i personaggi dello stesso ruolo;
- il personaggio è la principale fonte di identità;
- build/talenti/gadget devono introdurre trade-off laterali, non upgrade puri;
- non creare una sottoclasse C++ per ogni combinazione se una definizione data-driven è sufficiente;
- mantenere il principio: **C++ definisce cosa è possibile; dati/Blueprint quale variante viene usata**.

---

# 5. MOVE E DASH SONO DUE COSE DIVERSE

Consolidare esplicitamente:

## Move

Movimento normale nella fase `MOVE` finale.
Può avere profili come, dove già previsti dal progetto:

- Sneak;
- Normal Move;
- Sprint.

Questi profili possono modificare:

- distanza;
- costo;
- rumore;
- interazione col terreno;
- stabilità;
- esposizione;
- possibilità di Pivot finale.

## Dash

Movimento speciale nella fase `DASH`, prima di Blast.
La famiglia può includere concetti come:

- Dash;
- Charge;
- Leap;
- Blink;
- Reposition speciale;

ma la tassonomia effettiva deve seguire quella già canonica nella repository.

Non trattare Sprint come sinonimo di Dash.

---

# 6. FACING COME STATO TATTICO DI PRIMA CLASSE

Decisione centrale:

> Il Facing NON è una proprietà accessoria del Move. È uno stato tattico persistente dell'unità che può essere modificato da più categorie di eventi durante il turno.

La mappa è esagonale, quindi il modello naturale è un Facing discreto su **6 direzioni**, con step di **60°**.

Esempio concettuale:

```text
FacingIndex = 0..5
1 step = 60°
2 step = 120°
3 step = 180°
```

Con sei direzioni, una capacità di ±3 step consente di raggiungere qualsiasi orientamento finale.

Il Facing deve stare nello stato logico/snapshot dell'unità e deve essere risolto dal simulatore, non dall'animazione.

---

# 7. FONTI POSSIBILI DI CAMBIO FACING

Il consolidamento deve riconoscere almeno queste famiglie:

```text
FACING
  ↑
  ├─ Movement micro-step
  ├─ End-of-Move Pivot
  ├─ Dash / special movement
  ├─ Attack / ability execution
  ├─ Reaction
  ├─ Brace / Guard / Defense
  ├─ Interact
  ├─ Forced Movement
  ├─ Impact / displacement effect
  ├─ Terrain / transition / environment
  ├─ Status / temporary effect
  └─ Direct facing-control ability
```

Non tutte devono essere implementate subito.
Devono però essere modellabili senza introdurre eccezioni hard-coded.

---

# 8. END-OF-MOVE PIVOT

Alla fine del Move alcuni personaggi possono correggere il proprio orientamento più di altri.

Esempi di scala da prototipare:

```text
0 step   = nessuna rotazione finale
±1 step  = max 60°
±2 step  = max 120°
±3 step  = max 180° / qualsiasi Facing esagonale
```

Questa capacità NON deve essere automaticamente uguale per ruolo.

Archetipi utili per test, non necessariamente nomi canonici:

- Heavy: Pivot ridotto;
- Standard: Pivot medio;
- Agile: Pivot ampio.

Effetto tattico desiderato:

> La stessa cella finale può avere valore diverso in base al lato da cui ci si arriva e al Facing ottenibile.

Quindi un percorso leggermente più lungo può essere tatticamente migliore se permette di entrare nella cella con orientamento favorevole.

Questo punto ha conseguenze su path preview e potenzialmente su pathfinding/orientation state: creare issue/decisione se non è già coperto.

---

# 9. MOVEMENT FACING DURANTE I MICRO-STEP

Direzione proposta da validare, NON canonizzare ciecamente se non esiste ancora una decisione esplicita:

- durante il normale movimento l'unità può orientarsi secondo la direzione della transizione corrente;
- reaction, Overwatch, cover e altri sistemi che si attivano a micro-step devono leggere il Facing effettivo di quello specifico boundary;
- il Pivot finale avviene solo al termine del movimento e non retroattivamente.

Se la repository ha già una regola diversa, preservarla e segnalare il conflitto.

Serve test dedicato.

---

# 10. DASH END FACING

Dash deve avere una policy di Facing separata dal Move.

Esempio concettuale:

```text
MoveEndPivotMaxSteps
DashEndPivotMaxSteps
```

Un Charge pesante potrebbe:

- muoversi in linea;
- terminare nella direzione del Dash;
- avere 0 o 1 step di correzione.

Un Dash agile potrebbe:

- terminare con rotazione molto più ampia;
- essere usato anche per ricostruire un angolo di attacco/Overwatch.

Non obbligare tutte le forme di Dash alla stessa policy.

---

# 11. ATTACCHI E ABILITÀ POSSONO MODIFICARE IL FACING

Un attacco può girare il personaggio.

Serve una `Facing Policy` dichiarativa per azione/ability, concettualmente equivalente a policy come:

```text
KeepFacing
FaceTarget
FaceAttackDirection
LimitedTurn(NSteps)
FreeTurn
RestorePreviousFacing
```

I nomi reali devono seguire le convenzioni del progetto.
NON implementare questi enum se non è scope della task corrente: documentare la necessità e creare backlog/issue se assente.

Conseguenza importante:

> Attaccare può cambiare lo stato tattico successivo dell'unità.

Esempio:

- Wraith può riuscire a girarsi molto per sparare e restare rivolto verso il nuovo settore;
- Riktor può avere capacità di rotazione limitata e quindi non poter attaccare facilmente un bersaglio alle spalle.

La validità dell'attacco deve essere deterministica e spiegabile.

---

# 12. REACTION CHE MODIFICANO IL FACING

Una reaction può far girare l'unità.

Esempi:

- Return Fire ruota verso la sorgente dell'attacco;
- una reaction difensiva orienta lo scudo verso l'attaccante;
- una reaction può mantenere il Facing se rappresenta un settore già preparato;
- una reaction può avere limite massimo di rotazione;
- il nuovo Facing rimane valido per gli eventi successivi dello stesso turno, salvo policy esplicita di restore.

Questo è particolarmente importante perché il resolver può avere più Decision Boundary nello stesso round.

La reaction non deve usare l'animazione come autorità.

---

# 13. OVERWATCH E FACING

Overwatch è direzionale e deve essere coerente con il Facing.

Principi già consolidati da preservare:

- Overwatch viene preparata in Planning;
- è una reaction armata;
- produce 0..N Reaction Opportunities;
- Fast Reaction breve, baseline 3 s dove ancora canonico;
- HOLD perde quella opportunità ma può lasciare la reaction armata;
- niente informazione sul futuro della resolution al client;
- trigger simultanei nello stesso micro-step vengono raccolti correttamente;
- ordine deterministico;
- nessun leak degli intenti avversari.

Aggiornamento da consolidare:

> La geometria/settore di Overwatch deve dipendere dal Facing effettivo dell'unità e dalla policy della specifica action/profile.

Una unità non deve ruotare magicamente di 180° per consumare Overwatch se la sua policy non lo permette.

Reaction Facing e Overwatch Facing devono quindi essere documentati insieme.

---

# 14. BRACE / GUARD / DEFENSE E FACING

La difesa può essere direzionale.

Possibili comportamenti da supportare/documentare:

- orienta la difesa verso un settore;
- lock del Facing durante la stance;
- rotazione limitata quando si intercetta un attacco;
- protezione frontale forte / posteriore debole;
- Intercept che cambia Facing verso la traiettoria;
- capacità specifiche del personaggio di ruotare lo scudo.

NON trasformare automaticamente Brace in “+armor”.
Preservare il valore della geometria e della cover direzionale.

---

# 15. INTERACT E FACING

Una interazione con oggetto/mappa può richiedere Facing coerente.

Esempi possibili:

- console;
- valvola;
- leva;
- porta;
- terminale;
- dispositivo ambientale.

Policy possibile:

- deve già essere rivolto verso l'oggetto;
- oppure l'Interact può ruotarlo entro un limite;
- il Facing finale può rimanere quello dell'interazione.

Questa è una proposta da validare con UX/gameplay, non una regola da hardcodare senza test.

---

# 16. FORCED MOVEMENT / IMPACT / DISPLACEMENT

Gli effetti di spostamento devono dichiarare cosa accade al Facing.

Esempi di policy concettuali:

```text
PreserveFacing
FaceMovementDirection
FaceAwayFromSource
FaceSource
RotateSteps(+/-N)
SetFacing(Direction)
```

Esempi di gameplay:

- push leggero: preserva Facing;
- knockback pesante: può orientare nella direzione di lancio;
- effetto “spin”: ruota 60°/120°;
- throw: può impostare Facing specifico.

Niente rotazioni casuali non deterministiche.

---

# 17. ESSERE COLPITI NON DEVE GIRARE SEMPRE AUTOMATICAMENTE

Non introdurre una regola globale “ogni colpo ruota il bersaglio”.

Invece alcune abilità possono avere un effetto esplicito di manipolazione del Facing.

Questa può diventare una forma di **crowd control direzionale**.

Esempio di combo desiderabile:

1. Riktor protegge un settore frontalmente;
2. un alleato/nemico usa un effetto che lo ruota di 60°;
3. si apre una nuova linea di tiro;
4. un secondo personaggio sfrutta la geometria cambiata.

Il valore non è solo in HP, ma nella manipolazione della geometria tattica.

---

# 18. TERRAIN / MAP / TRANSITIONS CHE POSSONO MODIFICARE IL FACING

Il sistema deve poter rappresentare effetti come:

- ghiaccio: riduzione o annullamento del Pivot finale;
- acqua profonda: rotazione più difficile, se il design la usa;
- tunnel stretto: orientamento vincolato alla direttrice;
- scala/ladder: Facing obbligato durante la transizione;
- ponte stretto / choke: orientamento limitato dalla transizione, se previsto;
- conveyor / corrente: forced movement con policy Facing;
- ascensore/porta: eventuale Facing di uscita dichiarato dalla transizione.

Non introdurre tutti questi comportamenti nel vertical slice se non sono già scope.
Aggiornare feature/backlog per rendere il modello estendibile.

---

# 19. STATUS / TEMPORARY EFFECTS CHE POSSONO MODIFICARE IL FACING

Separare capacità di muoversi e capacità di ruotare.

Esempi concettuali da usare per design/test:

```text
Rooted:
  Move = 0
  Pivot = normale

Immobilized:
  Move = 0
  Pivot limitato

Stunned:
  Pivot = 0

Suppressed:
  PivotMaxSteps ridotto

Focused / Anchored:
  Facing lock
  beneficio associato
```

Questi non sono valori canonici automatici: verificare le definizioni esistenti.

---

# 20. DIRECT FACING CONTROL COME EFFETTO RIUTILIZZABILE

Il sistema dovrebbe poter esprimere concettualmente effetti come:

```text
FaceTarget
FaceSource
FaceAwayFromSource
FaceDirection
RotateClockwise(N)
RotateCounterClockwise(N)
LockFacing
UnlockFacing
RestorePreviousFacing
```

Lo scopo è evitare abilità hard-coded specifiche per singolo personaggio.

Se il framework effetti corrente non lo supporta, aggiungere una feature/issue dedicata, non una patch locale nella documentazione di un eroe.

---

# 21. MOVEMENT SIGNATURE DEL PERSONAGGIO

Direzione da consolidare nel character design/data model:

Ogni personaggio può avere una **Movement / Orientation Signature** leggibile e data-driven.

Campi concettuali:

```text
MoveRange
MoveProfiles
TerrainAffinity
MovementNoise

MoveEndPivotMaxSteps
StationaryPivotMaxSteps   // decisione ancora da chiudere

DashType / DashProfile
DashRange
DashEndPivotMaxSteps

ForcedMovementResistance
FacingRestrictions / modifiers
```

Non usare necessariamente questi nomi esatti nel codice.
Prima verificare i tipi già presenti.

---

# 22. BASE ACTION SIGNATURE DEL PERSONAGGIO

Oltre alle quattro abilità speciali, il personaggio dovrebbe avere un profilo riconoscibile per:

```text
Move
Dash
Basic Attack
Brace / Guard
Overwatch
Interact
Wait
```

Obiettivo:

> Anche con skill speciali in cooldown, Gadget deve continuare a sentirsi Gadget, Phase deve sentirsi Phase, Riktor deve sentirsi Riktor, Wraith deve sentirsi Wraith.

Le azioni base non devono diventare “quinta/sesta ability gratuita” carica di troppi effetti.
Devono rimanere leggibili e affidabili.

---

# 23. ROSTER v0.1 — DIREZIONI DI PROTOTIPO

Roster v0.1 corrente:

- Gadget
- Phase
- Riktor
- Wraith

Questi valori sono **ipotesi iniziali da scenario/playtest**, NON canonizzarli come bilanciamento finale senza conferma.

## 23.1 Move / Pivot / Dash — matrice candidata

| Personaggio | Move identity | Pivot fine Move | Dash identity | Pivot fine Dash |
|---|---|---:|---|---:|
| Gadget | standard/tecnico | candidato ±120° | corto/tecnico | candidato ±120° |
| Phase | fluido | candidato ±120° | molto manovrabile | candidato ±180° |
| Riktor | pesante | candidato ±60° | charge/ram | candidato 0–60° |
| Wraith | agile/predittivo | candidato ±180° | reposition rapido | candidato ±180° |

Questa tabella va usata come base per test e matrice di design, non come “valore già approvato”.

## 23.2 Identità azioni base — direzioni candidate

### Gadget

- Move: cerca geometrie/terreni utili alla conduzione;
- Dash: tecnico, utile a ricostruire linee elettriche;
- Basic Attack: semplice attacco elettrico lineare, non sovraccaricarlo di effetti;
- Brace/Guard: possibile interazione con grounding/carica;
- Overwatch: settore/linea conduttiva;
- Interact: forte affinità con dispositivi elettrici;
- Wait: eventuale conservazione/stabilizzazione risorsa, solo se non rompe action economy.

### Phase

- Move: interazione favorevole con Wet/Water;
- Dash: fluido/manovrabile, potenzialmente lascia/scorre su acqua se già previsto dal kit;
- Basic Attack: getto leggibile, possibile displacement leggero;
- Brace/Guard: stabilità/flow difensivo;
- Overwatch: controllo choke/zone bagnate;
- Interact: forte affinità con valvole/pompe/acqua;
- Wait: eventuale gestione risorsa acqua, se coerente con data model.

### Riktor

- Move: pesante, forte stabilità ma Pivot ridotto;
- Dash: ram/charge, direzionale;
- Basic Attack: semplice e affidabile;
- Brace/Guard: forte identità frontale;
- Overwatch: cono corto/largo e difensivo;
- Interact: affinità con cover/strutture/porte;
- Wait: possibile consolidamento stance solo se già supportato dal design.

### Wraith

- Move: agile e orientato a ottenere angoli;
- Dash: forte capacità di reposition e Pivot;
- Basic Attack: preciso/predittivo;
- Brace/Guard: focus/line control più che tanking;
- Overwatch: settore più stretto/lungo, intercettazione;
- Interact: standard salvo fantasy già definita;
- Wait: eventuale mantenimento Focus se esiste come meccanica.

ATTENZIONE:
Non trasformare questi esempi in abilità canoniche se i CharacterDefinition correnti hanno già regole diverse.
Usali per costruire una matrice di design e identificare cosa deve essere deciso.

---

# 24. ROSTER v0.2

Roster v0.2 corrente:

- Steel
- Aurora
- Murdock
- Kwang

Questo consolidamento deve preparare lo stesso schema di Base Action Signature / Movement Signature anche per loro, ma NON inventare valori definitivi senza confrontare le definizioni già consolidate nelle altre fonti del progetto.

Se esistono già pagine/wiki/matrici per v0.2, aggiungere le colonne/righe necessarie per:

- Move profile;
- Dash profile;
- Move Pivot;
- Dash Pivot;
- Attack Facing Policy;
- Reaction Facing Policy;
- Brace/Guard Facing;
- Interact Facing;
- eventuali special rules.

---

# 25. FACING E PATHFINDING

Nuovo punto da rappresentare in roadmap/feature backlog:

> L'orientamento finale può rendere due path verso la stessa cella tatticamente diversi.

Domanda tecnica da valutare:

- il pathfinding autorevole deve restare puramente `CellId -> CellId` e il Facing viene validato alla fine?
- oppure alcune query devono usare uno stato esteso `(CellId, Facing)`?

NON scegliere una soluzione costosa senza audit.

Per il vertical slice può bastare:

1. path geometrico normale;
2. derivazione Facing dalle transizioni;
3. validazione del Pivot finale;
4. UI che mostra destinazioni/facing raggiungibili.

Se serve un A* orientation-aware, inserirlo in roadmap come incremento separato e misurabile.

Ricordare i budget esistenti per path query e preview.

---

# 26. FACING E LOS / COVER / PERCEPTION

Consolidare i collegamenti:

- Facing influenza cover direzionale;
- Facing influenza Overwatch e reaction direzionali;
- Facing può influenzare la percezione/cono visivo se la specifica corrente lo prevede;
- Facing può cambiare target legality e vulnerabilità da flank;
- quota, opacity, cover e LOS restano servizi separati;
- non introdurre un bonus danno automatico da high ground se non già approvato.

Verificare ADR/spec di percezione/facing attuali e aggiornarli senza duplicare regole.

---

# 27. UI / UX DA AGGIORNARE

La UI deve rendere il Facing leggibile già in Planning.

Aggiornamenti desiderati:

## Path ghost

Mostrare almeno:

- path;
- destinazione;
- **Facing finale previsto**;
- range di Pivot consentito;
- eventuale Facing obbligato da Dash/attack/reaction quando rilevante.

## Targeting

Quando si seleziona un attacco:

- mostrare se richiede rotazione;
- mostrare Facing risultante;
- mostrare target non raggiungibile per limite di rotazione con reason code chiaro.

## Reaction

Quando una reaction può ruotare:

- mostrare conseguenza di Facing nella Fast Reaction UI solo con informazioni legittime;
- non mostrare future opportunities.

## Certainty

Preservare:

- Confermato;
- Previsto;
- Incerto.

Gli intenti di Facing della propria squadra possono essere visualizzati come preview team-only.
Il Facing futuro pianificato degli avversari NON deve essere leakato.

---

# 28. NETWORK / PRIVACY

Il Facing corrente pubblico durante la resolution può essere parte dello stato osservabile secondo le regole di visibilità.

Il **Facing pianificato futuro** è invece parte dell'intento e deve seguire le stesse regole di privacy del planning:

- canonical intent completo solo server;
- preview del Facing finale alleato solo team/owner secondo policy;
- niente proprietà globali replicate con facing futuro avversario;
- niente warning client derivati dal planning avversario;
- risultati risolti possono essere pubblicati quando diventano legittimamente osservabili.

Aggiungere test anti-leak specifico con canary Facing/EndFacing se il protocollo di intent viene esteso.

---

# 29. SIMULATORE / SNAPSHOT / TURNLOG

Facing deve essere logico e deterministico.

Lo snapshot unità deve poter rappresentare il Facing corrente.

Il resolver deve produrre eventi espliciti quando il Facing cambia.

Esempio concettuale:

```text
FacingChanged
UnitId = Riktor
From = East
To = NorthEast
Reason = Move.EndPivot
```

oppure:

```text
FacingChanged
UnitId = Wraith
From = South
To = North
Reason = Reaction.ReturnFire
```

Reason code utili:

- MoveStep;
- MoveEndPivot;
- Dash;
- Attack.FaceTarget;
- Reaction;
- Guard;
- Interact;
- ForcedMovement;
- Impact;
- Terrain;
- Status;
- AbilityEffect.

Usare la nomenclatura reale del progetto.

Regola fondamentale:

> L'animazione può girare il mesh, ma non può decidere il Facing competitivo.

Stesso snapshot + rules/version + seed + intent deve produrre stessa sequenza di FacingChanged e stessi hash finali.

---

# 30. FEATURE REGISTRY — AGGIORNAMENTI RICHIESTI

Trova il vero registry e aggiorna lì le feature, senza creare duplicati.

Le capability che devono risultare tracciabili sono almeno:

1. Base Action Character Profiles;
2. Movement Signature;
3. Dash Signature;
4. Facing as Authoritative Unit State;
5. End-of-Move Pivot;
6. Dash End Facing;
7. Action Facing Policies;
8. Reaction Facing Policies;
9. Directional Guard / Brace Facing;
10. Interact Facing Policy;
11. Forced Movement Facing Policy;
12. Direct Facing Manipulation Effects;
13. Facing-aware Overwatch;
14. Facing-aware Planning Preview;
15. Facing TurnLog / Explainability;
16. Facing Determinism Tests;
17. Planned Facing Privacy / Anti-Leak Test;
18. eventuale Orientation-aware Pathfinding, se approvato come feature distinta.

Se il registry usa feature aggregate, NON frammentare artificialmente in 18 feature: inserire queste come sub-feature / acceptance criteria / child issue secondo il modello reale.

Ogni feature mostrata o linkata nella wiki deve poter risalire al suo stato nella roadmap/registry secondo la convenzione già decisa nel progetto.

---

# 31. ROADMAP — COME AGGIORNARLA

NON inventare una nuova roadmap.
Aggiorna quella corrente.

Per ogni capability sopra:

- individua milestone/checkpoint esistente appropriato;
- aggiungi dipendenze;
- indica cosa è “design/doc only”, cosa richiede runtime, cosa richiede UI, cosa richiede network/test;
- non anticipare implementazioni fuori scope del vertical slice;
- conserva l'ordine di rischio già adottato dal progetto.

Dipendenze concettuali:

```text
Facing State
   ↓
Facing Events / Resolver
   ↓
Move/Dash Pivot Policies
   ↓
Attack / Reaction Facing Policies
   ↓
Overwatch / Guard / Interact integrations
   ↓
UI Preview + Explainability
   ↓
Network Privacy tests
   ↓
Advanced orientation-aware pathfinding (solo se necessario)
```

---

# 32. ISSUE / EPIC

Prima cerca issue esistenti.

Aggiorna quelle già pertinenti quando possibile.
Crea nuove issue solo se manca un contenitore chiaro.

Possibili cluster, da adattare al progetto:

- Facing Core & Deterministic State;
- Movement/Dash Pivot Policies;
- Action/Reaction Facing Policies;
- Facing UI & Planning Preview;
- Facing + Overwatch/Guard;
- Facing + Pathfinding;
- Facing Privacy & Network Tests;
- Base Action Character Profiles / roster matrix.

Ogni issue deve avere acceptance criteria verificabili e riferimenti a feature registry / roadmap / scenari.

---

# 33. WIKI SEPARATA — AGGIORNAMENTI

La wiki è nella repository separata:

`refactor-tactics-main.wiki`

Aggiornare le pagine esistenti appropriate, senza creare pagine duplicate solo perché i nomi di questo prompt sono comodi.

Informazioni player-facing da consolidare:

- differenza Move vs Dash;
- ordine `Prep -> Dash -> Blast -> Move`;
- Facing su 6 direzioni;
- rotazione finale/Pivot;
- personaggi con capacità di rotazione diverse;
- un attacco può cambiare Facing;
- una reaction può cambiare Facing;
- guardia/Overwatch dipendono dal Facing;
- forced movement/alcuni effetti possono ruotare;
- stesso esagono + Facing diverso = stato tattico diverso;
- preview del Facing nel Planning;
- niente esposizione di dettagli tecnici server-only.

Per pagine personaggi:

- aggiungere una sezione coerente per Base Action Signature / Movement Signature se la struttura wiki lo consente;
- NON sostituire le 4 skill con le azioni base;
- le abilità restano proprietà del singolo personaggio;
- combo/sinergie restano esempi separati, non categorie di abilità.

Ogni feature wiki che il progetto vuole tracciare deve avere riferimento al Feature Registry / Roadmap secondo la convenzione già stabilita.

---

# 34. XLSX / MATRICI

Verificare se esistono fogli Excel usati come fonte dati o come matrice di bilanciamento/wiki.

Se sì, aggiornare la **source of truth**, non soltanto il markdown derivato.

Aggiungere o estendere una matrice per personaggio con colonne equivalenti a:

| Character | Role | MoveProfile | MoveRange | MovePivot | DashProfile | DashRange | DashPivot | BasicAttackFacing | GuardFacing | OverwatchFacing | InteractFacing | ReactionFacing | Notes |
|---|---|---|---:|---:|---|---:|---:|---|---|---|---|---|---|

Aggiungere valori v0.1 solo come:

- Approved;
- Proposed;
- TBD;

secondo lo stato reale.

Non convertire le ipotesi di questo handoff in valori approvati.

Se il Feature Registry o la wiki vengono generati da XLSX, aggiornare il foglio sorgente e rigenerare gli artefatti secondo il workflow esistente.

---

# 35. SCENARI DI VALIDAZIONE DA CREARE / AGGIORNARE

Integrare nel sistema scenari esistente.
Non creare una seconda cartella di test se ce n'è già una.

Creare scenari minimi e leggibili almeno per:

## FACING-01 — Move Pivot 60°

- unità Heavy/Riktor-like;
- arriva in una cella;
- prova Pivot di 60° -> valido;
- prova Pivot di 120°/180° -> invalido;
- reason code verificabile.

## FACING-02 — Move Pivot 180°

- unità Agile/Wraith-like;
- raggiunge la stessa cella;
- può scegliere qualsiasi dei sei Facing finali.

## FACING-03 — Same Cell, Different Entry Angle

- due path raggiungono la stessa cella;
- Facing derivato diverso;
- solo uno consente una certa difesa/azione successiva.

## FACING-04 — Attack Rotates

- attacco con `FaceTarget`;
- verifica FacingChanged;
- verifica Facing persistente dopo l'attacco.

## FACING-05 — Attack Rotation Limit

- bersaglio oltre il massimo turn consentito;
- azione invalidata o fizzle secondo policy;
- reason code.

## FACING-06 — Reaction Rotates

- attacco da lato/posteriore;
- reaction legale ruota l'unità;
- evento successivo nello stesso turno usa il nuovo Facing.

## FACING-07 — Overwatch Does Not Auto-Snap

- target entra fuori dal settore;
- Overwatch non può ruotare magicamente se policy non lo permette;
- nessun trigger illegale.

## FACING-08 — Return Fire Facing

- reaction ruota verso attacker;
- contrattacco;
- Facing finale coerente.

## FACING-09 — Forced Movement Preserve

- push con PreserveFacing;
- cella cambia;
- Facing resta invariato.

## FACING-10 — Impact Rotate

- colpo con effetto RotateSteps(1);
- target ruota esattamente 60°;
- deterministico.

## FACING-11 — Directional Guard

- Guard protegge davanti;
- attacco frontale ridotto/intercettato;
- attacco posteriore non riceve lo stesso beneficio.

## FACING-12 — Interact Facing

- unità interagisce con device;
- verifica policy di rotazione o requisito Facing.

## FACING-13 — Ice / Terrain Pivot Restriction

- stesso Move su terreno normale e ghiaccio;
- Pivot finale differente solo se questa regola viene approvata.

## FACING-14 — Status Pivot Restriction

- status limita rotazione;
- Move può essere legale ma Facing finale no.

## FACING-15 — Stationary Pivot

- unità non si muove;
- verifica la futura regola `StationaryPivotMaxSteps`.
- Questo scenario può restare Pending fino alla decisione di design.

## FACING-16 — TurnLog Explainability

- più cambi Facing nello stesso turno;
- ogni cambio ha source/reason code corretto;
- ordine stabile.

## FACING-17 — Determinism Repeat

- stessa fixture eseguita molte volte;
- stesso TurnLog/StateHash/LogHash.

## FACING-18 — Planned Facing Privacy

- Team A pianifica un Facing finale riconoscibile/canary;
- client Team B non deve ricevere quel dato durante planning;
- dopo resolution può ricevere solo stato legittimamente osservabile.

## FACING-19 — Team Preview

- alleato vede path + Facing finale dell'alleato;
- sequence/rate limit coerenti col sistema preview;
- nemico non lo riceve.

## FACING-20 — UI Ghost

- path ghost mostra destinazione e facing finale;
- warning corretto se Pivot impossibile.

---

# 36. TEST AUTOMATICI

Aggiornare la lista test reale del progetto includendo almeno:

## Core Automation

- Facing index / normalization su 6 direzioni;
- delta minimo tra Facing;
- clamp/validation Pivot steps;
- deterministic facing transition;
- forced movement facing policies;
- action facing validation;
- snapshot serialization/hash con Facing.

## Resolver Golden Tests

- Move -> Pivot;
- Dash -> Facing;
- Attack -> Facing;
- Reaction -> Facing;
- più FacingChanged nello stesso turno;
- collisioni/reaction con facing intermedio.

## Functional / PIE

- ghost finale;
- directional cover;
- Overwatch cone;
- Guard front/back;
- rotate-on-attack;
- reaction-facing.

## Network

- team-only planned Facing preview;
- canary anti-leak;
- stale sequence;
- reconnect/state sync se previsto dalla milestone.

## Packaged

- almeno uno scenario Facing end-to-end in build packaged secondo Definition of Done del progetto.

---

# 37. AUTOMATED SCENARIO TEST HARNESS

Se il progetto ha già il sistema/roadmap `RT Automated Scenario Test Harness`, integra questi scenari lì.

Principio obbligatorio:

- UI umana;
- test scenario;
- bot;
- replay;

devono produrre intent/decisioni verso lo stesso planning/resolver.

Niente scorciatoie test tipo:

```text
SetActorRotation
```

se il gameplay reale deve passare da un Facing Intent / resolver event.

Il report machine-readable deve poter spiegare:

- expected facing;
- actual facing;
- turn;
- phase;
- micro-step;
- event/reason code;
- source action/reaction/effect.

---

# 38. DECISIONI ANCORA APERTE — NON INVENTARE

Questi punti vanno evidenziati e, se non già decisi altrove, registrati come decisione aperta / issue / playtest question:

1. nome canonico di `Pivot` / `Reorient` / altra terminologia per rotazione in-place;
2. evitare collisione terminologica con `Reposition` usato come movimento speciale;
3. esatto Facing durante ogni micro-step di Move;
4. `StationaryPivotMaxSteps`: quanto può girarsi una unità che non si muove?;
5. valori Move Pivot per ogni personaggio;
6. valori Dash Pivot per ogni personaggio;
7. quali Basic Attack usano FaceTarget / LimitedTurn / KeepFacing;
8. quali Reaction possono ruotare;
9. Interact richiede Facing o lo modifica automaticamente?;
10. quali forced movement preservano o cambiano Facing;
11. quali status limitano il Pivot;
12. quali terreni/transizioni limitano Facing;
13. se/quanto il pathfinding deve diventare orientation-aware;
14. se una action che ruota per eseguire ritorna al Facing precedente o mantiene quello nuovo;
15. differenza precisa tra player-facing “Dash è un'azione base” e classificazione tecnica del Dash.

NON chiudere questi punti arbitrariamente.
Se il progetto possiede già una decisione più recente, usa quella e aggiorna questo elenco di conseguenza.

---

# 39. ACCEPTANCE CRITERIA DEL CONSOLIDAMENTO

Il task è completato solo se:

1. non esistono due fonti concorrenti per le stesse regole;
2. Move e Dash sono distinti correttamente;
3. Facing è descritto come stato autorevole e non animazione;
4. End Pivot è documentato;
5. action/reaction facing sono tracciati come capability;
6. feature registry reale è aggiornato;
7. roadmap reale contiene le feature/sub-feature necessarie;
8. issue/epic sono aggiornati senza duplicati;
9. wiki separata è aggiornata;
10. pagine personaggi possono rappresentare Base Action / Movement Signature;
11. scenari di validazione sono registrati;
12. test PIE/Automation/Network necessari sono registrati;
13. eventuali XLSX sorgente sono aggiornati quando applicabile;
14. privacy del planned Facing è esplicitamente coperta;
15. TurnLog/explainability del Facing è coperto;
16. tutti i valori ancora solo proposti restano marcati Proposed/TBD;
17. ogni nuova feature wiki ha collegamento allo stato roadmap/registry secondo la convenzione esistente;
18. nessuna vecchia informazione viene eliminata senza archival/provenance quando serve.

---

# 40. OUTPUT FINALE RICHIESTO A CLAUDE

Al termine restituisci un report strutturato con:

## A. Audit

- repository/versione UE rilevata;
- roadmap canonica trovata;
- feature registry canonico trovato;
- struttura wiki trovata;
- sistema scenari/test trovato;
- XLSX sorgente rilevanti.

## B. File modificati

Per ogni file:

```text
path
motivo
cosa è cambiato
```

Separare:

- repo principale;
- wiki;
- spreadsheet/data sources.

## C. Feature Registry

Per ogni feature/sub-feature modificata:

```text
Feature ID
Titolo
Stato
Roadmap location
Wiki reference
Issue/Epic
Scenario/Test coverage
```

## D. Roadmap

- checkpoint/milestone aggiornati;
- dipendenze;
- cosa entra nel vertical slice;
- cosa resta backlog.

## E. Issue / Epic

- issue aggiornate;
- issue create;
- issue duplicate evitate/chiuse/collegate.

## F. Wiki

- pagine aggiornate;
- pagine create solo se davvero necessarie;
- link registry/roadmap aggiunti.

## G. Scenari/Test

Tabella:

```text
Scenario/Test
Tipo
Feature coperta
Expected
Status
```

## H. Matrici XLSX

- workbook/sheet aggiornati;
- nuove colonne/righe;
- stato Approved/Proposed/TBD.

## I. Decisioni aperte

Elenca esclusivamente quelle non risolte dalle fonti reali della repository.

## J. Diff concettuale finale

Riassumi in massimo 15 punti il nuovo canone risultante, distinguendo:

- APPROVED/CANONICAL;
- PROPOSED FOR PLAYTEST;
- OPEN/TBD.

---

# 41. GUARDRAIL FINALI

- Non inventare API Unreal.
- Non implementare gameplay fuori scope solo per “completare” la documentazione.
- Non trasformare proposte di bilanciamento in canon.
- Non creare un secondo sistema Facing parallelo.
- Non usare l'animazione come autorità.
- Non far dipendere risultati dal frame rate.
- Non usare ordine implicito di `TMap/TSet` per risultati competitivi.
- Non leakare Facing/path/target futuri del planning nemico.
- Non mettere intenti completi in GameState/PlayerState globalmente replicati.
- Non creare file registry/roadmap duplicati.
- Non modificare la wiki principale se la source of truth è un XLSX senza aggiornare il source.
- Non raggruppare abilità per combinazioni di personaggi: le abilità appartengono al personaggio; combo/sinergie sono esempi separati.
- Conserva ID stabili, versioning, hash e validator.
- Ogni feature è Done solo secondo la Definition of Done reale del progetto: runtime previsto, privacy, log/debug, test automatico e packaged build quando applicabile.

