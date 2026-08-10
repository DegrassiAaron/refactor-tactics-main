# RefactorTactics — FULL CHAT CONSOLIDATION
## Griglia esagonale, geometria architettonica, muri, aperture, cover, LOS, traversal, occupazione, collisioni, reaction/intel, verticalità, distruzione, acqua ed elettricità
### Handoff completo per Claude

**Data:** 2026-08-10  
**Scopo:** consolidare **l'intero focus di design discusso in questa chat**, non solo l'ultima parte su traversal/acqua.  
**Destinatario:** Claude / Claude Code  
**Uso richiesto:** aggiornamento documentazione, Wiki, Roadmap, Feature Map, Scenario Map, Editor Map, backlog GitHub/repository, epic/issue, validator e test.

---

# A. ISTRUZIONE DI PREVALENZA

Questo documento raccoglie le decisioni esplicite prese nella conversazione corrente su:

- griglia esagonale;
- geometria delle direttrici;
- relazione tra hex e muri;
- supporto ad angoli a 90°;
- centri degli hex come riferimenti/vertici della geometria;
- celle occupabili/bloccate;
- porte, aperture, finestre e breach;
- cover a 6 settori;
- LOS e quota;
- wall profiles;
- Vault e traversal;
- occupazione e collisioni simultanee;
- assenza di ZoC universale;
- Overwatch completamente nascosto;
- reveal/intel;
- Layer e verticalità;
- cadute, Push/Knockback e displacement;
- crolli strutturali;
- macerie;
- acqua, profondità, flooding e correnti;
- propagazione elettrica sulla rete d'acqua.

Quando c'è conflitto con documentazione precedente:

**questa conversazione prevale**.

Non usare vecchie assunzioni come:
- muro coincidente col lato dell'esagono;
- porta derivata da larghezza/numero di lati;
- alleati attraversabili di default;
- high ground con bonus numerici automatici;
- Overwatch visibile/telegraphed di default;
- mesh/Chaos usati come autorità competitiva.

---

# B. GRIGLIA ESAGONALE — PRINCIPIO FONDAMENTALE

## LOCKED

RefactorTactics resta un gioco in cui **ci si muove su esagoni**.

Il sistema tattico discreto è il riferimento assoluto:

- una unità occupa sempre un esagono;
- una unità si sposta sempre da un esagono a un altro;
- niente posizione tattica continua;
- niente sub-cell;
- niente half-cell;
- niente footprint continuo come autorità;
- niente movimento "tra due hex" come stato logico permanente.

La struttura base resta:

```cpp
FRTCellId
{
    X;
    Y;
    Layer;
}
```

`Layer` distingue posizioni sovrapposte:
- Ground;
- Bridge;
- Roof;
- Tunnel;
- UpperFloor;
- LowerFloor;
- altri livelli authored.

La griglia non deve essere interpretata come una piastrellatura architettonica dove i muri sono obbligatoriamente sui bordi delle celle.

---

# C. GEOMETRIA ARCHITETTONICA E DIRETTRICI

## LOCKED

La geometria architettonica deve essere **indipendente dai bordi dell'esagono**.

Principio:

> La griglia decide le posizioni tattiche. La geometria decide muri, aperture, ostacoli, occlusione e relazioni ambientali. Il bake converte la geometria in regole discrete della griglia.

### C.1 Muri non sui lati dell'hex

È stato esplicitamente rifiutato il modello:

```text
HexEdge == WallSegment
```

Non costruire il sistema assumendo che un muro occupi "un lato dell'esagono".

Un muro può:
- attraversare graficamente una cella;
- passare vicino al centro;
- tagliare più celle;
- creare relazioni con più transition;
- creare un angolo che non coincide con un vertice/edge standard dell'hex.

### C.2 Geometria sulle direttrici

La geometria viene authored su **direttrici**.

L'obiettivo è consentire:
- allineamenti coerenti;
- muri lunghi;
- angoli architettonici;
- configurazioni leggibili;
- 90° reali/visivamente leggibili anche su griglia hex.

La geometria può quindi avere **più direzioni dell'insieme delle 6 direzioni di movimento**.

Le 6 direzioni di movement/facing/tactical cover restano separate.

### C.3 Supporto ai 90°

## LOCKED

La geometria deve poter generare angoli ortogonali/90°.

Non forzare tutte le costruzioni a seguire solo 60°/120° dell'hex.

Il reticolo/authoring geometrico può usare direttrici più ricche (es. famiglie di direzioni a passo più fine) purché il risultato gameplay venga poi ridotto a:
- celle;
- transition;
- cover sectors;
- LOS relations;
- opening relations.

### C.4 Centro dell'hex come riferimento geometrico

Decisione discussa:

> **Il centro di un esagono può essere considerato un vertice/punto del reticolo di costruzione dei muri.**

Questo NON significa che:
- ogni centro debba avere un muro;
- il muro sia sempre cell-centered;
- la cella sia automaticamente bloccata.

Significa che i centri possono essere usati come nodi/reference point utili per costruire segmenti geometrici coerenti.

---

# D. DIMENSIONE FISICA DELL'HEX

## LOCKED

La dimensione fisica dell'esagono deve essere **configurabile**.

Il vecchio riferimento a 1.5 m era solo indicativo.

Non hardcodare:
- 1.5 m;
- width fissa;
- side length fisso;
- door width -> N transitions tramite formula.

Qualsiasi rapporto tra geometria fisica e gameplay deve essere mediato da:
- profili;
- bake;
- dati discreti;
- override Editor.

---

# E. BAKE GEOMETRIA -> MAPSTATE

## LOCKED

La geometria architettonica non deve diventare un secondo sistema di navigazione.

Pipeline concettuale:

```text
Architectural Geometry
        ↓
Authoring / Bake
        ↓
Logical Map Relations
        ↓
FRTMapState
        ↓
Path / LOS / Cover / Resolver
```

Il runtime competitivo consuma:
- cell state;
- transition state;
- opening state;
- cover sectors;
- occlusion relations;
- structural state;
- water state;
- hazard state.

Il runtime **non** chiede alla mesh:
- "posso passare?";
- "questa porta è abbastanza larga?";
- "quanto muro è rimasto?";
- "la mia capsula collide?";
- "la fisica dice che sono caduto?".

---

# F. CELLA BLOCCATA VS MURO CHE PASSA NELL'HEX

## LOCKED

Caso 1 — muro solido passa **esattamente sul centro tattico** dell'hex:

```text
Cell = Blocked
```

Caso 2 — muro attraversa il poligono dell'hex ma **non** il centro:

```text
Cell may remain Walkable
```

ma può:
- bloccare alcune transition;
- influenzare LOS;
- generare cover locale;
- creare apertura/occlusione.

Questa distinzione è fondamentale.

---

# G. TRANSITION TATTICHE

## LOCKED

Il movimento autorevole è sempre una sequenza:

```text
Cell A -> Transition -> Cell B
```

Le transition sono dati di prima classe e possono avere:
- MovementPolicy;
- TraversalType;
- cost;
- requirements;
- blocker state;
- hazard;
- opening controller;
- structural dependency;
- water/flow relationship;
- revision.

Un muro può bloccare una transition senza bloccare la cella.

Una porta può controllare più transition.

Un traversal speciale può esistere su una transition che il Move normale non può usare.

---

# H. AUTHORING/EDITOR DELLA GEOMETRIA

Claude deve prevedere una Editor Map / toolset per:

- creare segmenti di muro sulle direttrici;
- snapping a punti/vertici autorizzati;
- junction;
- corner;
- 90°;
- aperture;
- preview dei centri cella;
- preview delle transition tagliate;
- preview cover sectors;
- preview LOS blockers;
- override manuali;
- validator.

Debug minimo:

```text
ShowCellCenters
ShowWallSegments
ShowWallJunctions
ShowOpeningBounds
ShowControlledTransitions
ShowControlledCells
ShowCoverSectors
ShowLOSBlockers
ShowOverrides
```

---

# I. DECISIONI DA NON REINTRODURRE

Sono esplicitamente scartate:

1. `Wall = Hex Edge`
2. "dividiamo il lato dell'hex in 3 pezzi"
3. numero di transition controllate da una porta derivato solo da larghezza fisica
4. movimento continuo all'interno dell'hex
5. doorway come posizione transit-only
6. alleati attraversabili di default
7. mesh collision come autorità del path
8. high ground = +accuracy universale
9. Overwatch nemico visibile di default
10. ZoC automatica per adiacenza
11. cover derivata automaticamente dall'altezza fisica della mesh

---

# J. NOTA SU ORIENTAMENTO / FACING VS GEOMETRIA

Le 6 direzioni tattiche restano importanti per:
- facing;
- cover sectors;
- incoming attack sector;
- Overwatch direction;
- directional defense.

La geometria architettonica può avere più direttrici.

Non confondere:
- `GeometryDirection`
con
- `HexDirection / FacingDirection`.

Il bake deve tradurre geometria più ricca in relazioni tattiche discrete.

---

# K. TRACCIABILITÀ: QUESTA È LA PARTE "MURI/GRIGLIA" CHE DEVE ESSERE AGGIUNTA ESPLICITAMENTE ALLA WIKI

Creare/aggiornare pagine dedicate:

1. **Hex Grid Fundamentals**
2. **Architectural Geometry on Hex Tactical Grid**
3. **Wall Direction Lattice**
4. **90 Degree Geometry on Hex Maps**
5. **Cell Center / Geometry Vertex Rules**
6. **Geometry Bake to Tactical Graph**
7. **Wall vs Cell Blocking**
8. **Wall vs Transition Blocking**
9. **Geometry Authoring and Overrides**

Queste pagine devono essere referenziate da:
- PDR Map/Pathfinding;
- Feature Map;
- Editor Map;
- Scenario Map;
- epic Geometry;
- test automation.

---

# L. SCENARI AGGIUNTIVI SPECIFICI PER GRIGLIA/MURI

Oltre agli scenari già elencati nel resto dell'handoff, aggiungere:

## SCN-GRID-01 Hex Center as Geometry Vertex
Verifica snapping/authoring e nessuna implicazione automatica di blocking.

## SCN-GRID-02 90 Degree Wall Corner
Due muri a 90° attraversano la griglia senza assumere hex-edge walls.

## SCN-GRID-03 Long Wall Across Multiple Hexes
Verifica celle walkable/bloccate e transition affected.

## SCN-GRID-04 Wall Crosses Hex Without Center
Cell walkable, alcune transition blocked.

## SCN-GRID-05 Wall Through Exact Center
Cell blocked.

## SCN-GRID-06 Parallel/Offset Walls
Due segmenti vicini producono bake distinto e deterministico.

## SCN-GRID-07 Wall Junction + Door Margin
Validator per opening troppo vicino a corner.

## SCN-GRID-08 Geometry Direction vs Hex Cover Sector
Geometria obliqua assegna uno o più dei 6 cover sectors.

---

# M. FEATURE MAP — AGGIUNTE ESPLICITE PER IL BLOCCO GRIGLIA/MURI

Assicurarsi che esistano feature distinte o sottosezioni chiare per:

- Hex Tactical Grid
- Geometry Direction Lattice
- 90° Architectural Corners
- Cell Center Geometry Vertices
- Wall Segment Authoring
- Geometry Bake
- Wall/Cell Intersection Rules
- Wall/Transition Intersection Rules
- Opening Binding
- Geometry Overrides
- Geometry Validator

Non nascondere tutto sotto una generica feature "Map".

---

# N. EPIC RACCOMANDATO — GEOMETRY FOUNDATION

Se non esiste già, creare o aggiornare un epic dedicato:

## EPIC — Hex Grid & Architectural Geometry Foundation

Scope:
- tactical hex topology;
- geometry direction lattice;
- 90-degree walls;
- wall segment authoring;
- cell center reference points;
- geometry bake;
- cell/block rules;
- transition block rules;
- editor preview;
- overrides;
- validator;
- automation scenarios.

Dipendenze:
- FRTCellId;
- FRTMapState;
- MapSubsystem;
- pathfinding graph;
- LOS service;
- cover service.

DoD:
- authorable in Editor;
- deterministic bake;
- no actor-per-cell;
- no mesh authority;
- debug overlays;
- tests;
- packaged map validation.

---

# O. RESUME / CONTEXT

Dopo questo blocco iniziale su griglia e muri, il documento continua con tutte le decisioni successive prese nella stessa conversazione:
- doors/openings;
- cover;
- LOS;
- Vault;
- hazard;
- occupancy;
- collisioni;
- reaction/intel;
- verticalità;
- fall/displacement;
- structural;
- water;
- connected-water electricity.



---

# PARTE II — TUTTE LE DECISIONI SUCCESSIVE DELLA CONVERSAZIONE

# 0. ISTRUZIONE PRINCIPALE PER CLAUDE

Questa nota NON è un brainstorming. Le decisioni marcate come **LOCKED / CONSOLIDATE** devono essere trattate come fonte più recente rispetto ai PDR precedenti quando c'è conflitto.

Ordine di prevalenza:

1. decisioni esplicite contenute in questo handoff;
2. decisioni esplicite già consolidate nelle chat/notes specialistiche recenti;
3. PDR correnti;
4. proposte/raccomandazioni precedenti;
5. materiale di ricerca.

Prima di modificare qualsiasi cosa:

- ispeziona la struttura reale del repository;
- trova i file attuali di Roadmap, Feature Map, Scenario Map, Editor Map e Wiki;
- trova epic/issue già esistenti su questi argomenti;
- **NON duplicare epic o issue**: aggiorna quelle esistenti se semanticamente equivalenti;
- conserva gli ID stabili già assegnati;
- preserva schema e convenzioni YAML/Markdown già adottati nel repository;
- se un vecchio documento contraddice una decisione qui sotto, aggiornalo oppure marcane chiaramente la parte come superseded/deprecated.

Documenti già esistenti da riallineare almeno:

- `RT_PDR_03_Architettura_UE5_v0.1`
- `RT_PDR_04_Networking_Privacy_v0.1`
- `RT_PDR_05_Simulazione_Deterministica_v0.1`
- `RT_PDR_06_Mappa_Pathfinding_v0.1`
- `RT_PDR_07_Abilita_Personaggi_GAS_v0.1`
- `RT_PDR_08_UI_UX_Coordinazione_v0.1`
- `RT_PDR_09_Dati_Validazione_Modding_v0.1`
- `RT_PDR_10_Roadmap_QA_Rischi_v0.1`
- `RT_PDR_11_Demo_v0.1`
- `RefactorTactics_Overwatch_FastReaction_Claude.md`
- `RefactorTactics_Rumore_Claude.md`
- `RefactorTactics_Balance_Matrices_v0.1.xlsx`

Il PDR Demo più vecchio contiene assunzioni che possono essere ormai obsolete (es. mappa piatta, alcune regole di cover, nomi personaggi, finestre reaction). Non propagare automaticamente quelle assunzioni contro le decisioni più recenti.

---

# 1. CONTESTO ARCHITETTURALE DA PRESERVARE

RefactorTactics è un tattico competitivo UE5 con:

- posizioni discrete su **griglia esagonale**;
- `FRTCellId = X, Y, Layer`;
- mappa come **grafo tattico 3D**;
- celle compatte centralizzate, non un Actor per ogni cella;
- archi/transizioni di prima classe;
- A* autorevole sul grafo;
- LOS, targeting, trajectory e pathfinding come servizi separati;
- C++ autorità per simulazione, rete, validazione e regole;
- Data Asset / Blueprint per varianti, configurazione e presentazione;
- snapshot immutabile;
- resolution deterministica per micro-step;
- `TurnLog` canonico;
- niente risultato competitivo deciso da fisica real-time, frame rate, montage o Chaos;
- stesso snapshot + rules + content hash + seed => stesso risultato;
- planning nemico privato mai replicato ai client avversari;
- UI con **Confermato / Previsto / Incerto**;
- Move volontario come fase finale, con Dash/spostamenti speciali nelle rispettive fasi;
- reaction generiche e Overwatch come caso concreto del framework reaction.

---

# 2. PRINCIPIO CARDINE DELLA GEOMETRIA

## LOCKED

**Gli esagoni definiscono le posizioni e la topologia tattica. La geometria architettonica NON coincide con i bordi degli esagoni.**

Non usare il modello:

`lato dell'hex == pezzo di muro`

È stato esplicitamente scartato.

La geometria dei muri può essere più ricca delle sei direzioni tattiche e deve supportare architetture leggibili, inclusi angoli a 90°. I centri degli esagoni possono essere usati come punti/vertici utili del reticolo di authoring dei muri.

Tuttavia, a runtime competitivo:

- una unità è sempre su un `FRTCellId`;
- si muove solo `Cell A -> Cell B`;
- niente sub-cell position;
- niente footprint continuo;
- niente "mezzo hex";
- niente movimento libero lungo la mesh;
- geometria e mesh vengono **baked/proiettate in proprietà discrete** di celle, transition, LOS, cover, aperture e hazard.

La mesh UE è presentazione/collisione visibile; **non è l'autorità delle regole competitive**.

La dimensione fisica dell'esagono deve essere configurabile. Non hardcodare 1.5 m.

---

# 3. MURO CHE INTERSECA UN HEX

## LOCKED

- Se geometria solida passa **esattamente sul centro tattico** di una cella: la cella è `Blocked`.
- Se il muro attraversa il poligono grafico dell'hex ma **non** il centro: la cella può restare `Walkable`.
- In quel caso il muro può comunque bloccare una o più transition in uscita.
- Nessun trace runtime sulla mesh decide se il personaggio può passare.
- Il risultato viene determinato/baked nel `FRTMapState`.

---

# 4. APERTURE: PORTE, FINESTRE, VARCHI

## 4.1 Binding apertura <-> transition

## LOCKED

Regola baseline: **auto-binding conservativo + override Editor**.

Una transition è controllata da un'apertura solo quando la linea logica center-to-center della transition attraversa chiaramente **l'interno** dell'apertura.

- tocco esatto di boundary/apertura/muro = `Blocked`;
- niente epsilon magici runtime;
- l'Editor può dichiarare un override esplicito;
- l'override deve diventare dato della mappa, versionato/validabile.

Una singola porta/apertura può controllare:

- `1..N ControlledTransitions`;
- `0..N ControlledCells`.

Non derivare il numero di transition dalla larghezza fisica con formule tipo `DoorWidth / HexSize`.

## 4.2 Porte ai corner

Baseline:

- porta/finestra standard non termina esattamente su corner/junction;
- deve esistere margine solido;
- in futuro si può introdurre un profilo dedicato tipo `CornerOpening/CornerDoor`.

Validator obbligatorio per violazioni.

## 4.3 Apertura sul centro cella

Se il centro della cella ricade dentro un'apertura valida:

- cella `Walkable`;
- una porta chiusa può rendere la `DoorwayCell` `Blocked`;
- una porta aperta ripristina lo stato base;
- una doorway cell è una cella tattica normale.

## 4.4 Niente TransitOnly

## LOCKED

Una doorway cell:

- può essere attraversata;
- può essere destinazione finale;
- può essere occupata;
- può essere usata per Brace, Overwatch, targeting ecc.

Non introdurre una categoria `TransitOnly` come baseline.

## 4.5 Chiusura porta con cella occupata

Se una porta controlla una o più celle e almeno una è occupata:

- l'azione di Close fallisce;
- porta resta aperta;
- niente push automatico;
- niente unità incastrata nella porta;
- loggare reason tipo `DoorBlockedByOccupant`.

## 4.6 Porta grande

Una porta/cancello grande può controllare più celle e più transition:

- closed => blocca tutte quelle previste dal profilo;
- open => ripristina lo stato base;
- close fallisce se una qualsiasi `ControlledCell` è occupata.

## 4.7 Swing dell'anta

L'anta fisica è **presentation only**.

Non blocca celle extra mentre ruota, salvo profilo speciale esplicito.

---

# 5. MOVEMENT POLICY DELLE APERTURE

## LOCKED

La larghezza fisica NON determina automaticamente il movimento.

Ogni apertura dichiara una `MovementPolicy` discreta.

Esempi:

- `StandardDoor` => pass quando open;
- `Window` => `Block` di default;
- `VaultableWindow` => `SpecialAction/SpecialTraversal`;
- `NarrowGap` => profilo specifico;
- porta 80 cm, 90 cm o 150 cm possono essere gameplay-equivalenti.

Per una finestra rotta:

- la distruzione non implica automaticamente passaggio;
- lo stato e il `WindowProfile` decidono;
- `SmallWindow Broken` può restare `Movement=Block` ma `Projectile=Pass`;
- `LargeWindow Broken` può diventare `SpecialTraversal`.

---

# 6. LOS E APERTURE

## LOCKED

LOS è un servizio separato dal pathfinding.

Baseline:

`LOS = SourceHex.Center -> TargetHex.Center`

Valutata contro **geometria logica** / stato delle aperture.

- `ClosedDoor` => blocca;
- `OpenDoor` => lascia passare se la linea attraversa chiaramente l'interno dell'apertura;
- tocco esatto del bordo => bloccato;
- override Editor ammesso e persistito.

**Non usare `ControlledTransitions` per decidere LOS.**

---

# 7. COVER — MODELLO CONSOLIDATO

## 7.1 La larghezza non genera cover

Una porta stretta non dà automaticamente cover.

Una normale porta aperta attraverso cui passa LOS:

`Cover = None`

Aperture speciali possono dichiarare:

`CoverPolicy = None | Light | Heavy`

es. firing slit, defensive window.

## 7.2 Cover quantizzata su 6 settori

## LOCKED

La cover tattica resta quantizzata sulle sei direzioni dell'hex.

Concetto:

`Cover[0..5]`

Non usare l'angolo continuo Source->Target come valore di cover.

L'attaccante viene mappato deterministicamente a **uno e un solo IncomingSector**.

Un singolo muro/ostacolo può assegnare cover a **più settori** della stessa cella.

Auto-bake + override Editor.

## 7.3 Tie esatto tra settori

Se la direzione cade esattamente sul confine di due settori:

- scegliere un solo settore;
- tie-break canonico fisso;
- mai scegliere il settore più favorevole a attacker/defender;
- UI mostra `IncomingSector` e cover applicata.

## 7.4 Cover locale al TargetHex

## LOCKED

La cover standard appartiene al contesto locale del `TargetHex`.

Un ostacolo lontano lungo la LOS:

- può bloccare/degradare LOS/trajectory;
- **non** concede automaticamente Cover.

Eccezioni tipo energy screen/intervening protection devono essere sistemi/profili espliciti.

## 7.5 Cover non stacka

Se più elementi proteggono lo stesso settore:

- si prende la cover più forte;
- `None < Light < Heavy`;
- niente `Light + Light = Heavy`;
- niente `Heavy + Light = SuperHeavy`.

Altri effetti difensivi restano separati (shield, damage reduction, barrier effect ecc.).

## 7.6 Facing indipendente

La cover ambientale **non dipende dal Facing dell'unità**.

Facing resta rilevante per:

- Brace;
- shield direzionali;
- counter;
- backstab;
- Overwatch;
- stance/abilità.

## 7.7 Copertura simmetrica

Per elementi standard:

- il muro/muretto può proteggere entrambi i lati.

Profili direzionali possono usare:

`CoverSides = FrontOnly`

es. deployable shield.

## 7.8 Cover e LOS via profili discreti

Non usare altezza fisica della mesh per tradurre automaticamente:

- Light Cover;
- Heavy Cover;
- LOS Block.

Esempio baseline:

- `FullWall`: LOS Block, normale Move Block;
- `LowWall`: LOS Pass, Cover tipicamente Heavy;
- `LightBarrier`: LOS Pass, Cover Light.

L'altezza fisica può servire all'authoring/visuale/validator ma non è l'autorità.

---

# 8. COVER E QUOTA

## LOCKED

La quota può degradare/annullare cover attraverso **regole discrete data-driven**.

Esempio concettuale:

`LowWall`
- same level => Heavy
- attacker +1 tactical elevation => Light
- attacker +2 => None

Non usare formule in centimetri.

Ogni profilo di cover/occluder può definire una `ElevationResponseProfile`.

La differenza di quota è firmata.

---

# 9. HIGH GROUND

## LOCKED

**Nessun bonus numerico universale automatico** per l'high ground.

No default:

- +Accuracy;
- +Damage;
- +Crit.

Il vantaggio deriva da:

- cover nemica degradata;
- migliore LOS;
- superamento di alcuni occluder;
- nuove linee di tiro/target;
- abilità/trait specifici.

Un marksman può avere un trait `HighGroundSpecialist`, ma non è regola globale.

---

# 10. FULL WALL E LOS DA QUOTA

## LOCKED

Un muro pieno può essere "visto sopra" da sufficiente quota, ma tramite **classi logiche discrete di altezza**.

Esempio:

`OcclusionHeightClass = Low | Medium | Tall | FullLevel`

LOS usa:

- SourceElevation;
- TargetElevation;
- OccluderHeightClass;
- OpeningState.

Niente raycast fisico UE come autorità.

---

# 11. UNITÀ COME OSTACOLI / COVER

## LOCKED

Una unità standard:

- occupa un hex;
- **non blocca LOS**;
- **non fornisce cover**;
- blocca il traversal perché occupa la cella.

Protezione tipo Bodyguard / Guardian / Shield deve essere abilità o stato esplicito.

---

# 12. OCCUPAZIONE — CORREZIONE IMPORTANTE

## LOCKED

**Non si possono attraversare gli alleati di default.**

Una unità occupa davvero un esagono.

Quindi:

- hex occupato da alleato => hard blocker;
- hex occupato da nemico => hard blocker;
- nessuno stacking;
- nessun pass-through ally standard.

Una ability speciale può derogare (Phase, Teleport, Swap ecc.).

Questa decisione prevale su qualsiasi vecchia proposta "allies pass through".

---

# 13. MOVIMENTO SIMULTANEO E COLLISIONI

## LOCKED

Resolver per micro-step:

1. `CollectMoveProposals`
2. `ResolveOccupancyDependencies`
3. `CommitSuccessfulTransitions`

## 13.1 Più unità -> stessa destinazione

Se due o più unità vogliono lo stesso hex nello stesso micro-step:

- tutte bloccate;
- nessuna initiative nascosta;
- niente ordine Actor/TMap.

Reason: `ContestedDestination`.

## 13.2 Catena che termina in spazio libero

Valida:

A->B  
B->C  
C->D  
D libero

Commit atomico:

C->D  
B->C  
A->B

Non è attraversamento di alleati: ogni occupazione cambia simultaneamente.

## 13.3 Catena bloccata

Se la catena termina su cella occupata da unità che non si muove:

- il blocco propaga all'indietro;
- nessuna compenetrazione temporanea.

## 13.4 Swap e cicli

Di default:

- A->B e B->A => bloccato;
- ciclo A->B, B->C, C->A => bloccato;
- anche tra alleati.

Servono ability/policy esplicite per Swap/Phase/Teleport.

## 13.5 PreEntry nella catena

Se una transition viene annullata da hazard/reaction `PreEntry`, la cella non viene liberata e le dipendenze a monte possono fallire.

## 13.6 Post-commit

Una reaction `OnEnter`/PostEntry che Roota una unità dopo il commit:

- non annulla retroattivamente il micro-step già concluso;
- impedisce il micro-step successivo.

---

# 14. NESSUNA ZONE OF CONTROL UNIVERSALE

## LOCKED

La semplice adiacenza a un nemico NON:

- riduce movimento;
- crea opportunity attack automatico;
- genera penalità invisibile;
- modifica pathfinding.

Il controllo dello spazio esiste solo tramite meccaniche esplicite:

- Overwatch;
- Intercept;
- Guard;
- Brace reaction;
- Suppression;
- Root field;
- Electric field;
- trap;
- stance;
- ability-specific reaction.

La UI deve mostrare il motivo concreto del pericolo, non "enemy nearby".

---

# 15. OVERWATCH: SEGRETEZZA ASSOLUTA

## LOCKED

Overwatch nemico è **completamente segreto** fino a trigger o reveal autorizzato.

Il client avversario non riceve:

- presenza dell'Overwatch;
- icona guarding;
- stance certa;
- cone;
- direction;
- cells;
- range;
- trigger;
- cost/path changes indiretti;
- warning derivati da Overwatch nascosto.

Attenzione ai leak indiretti: anche query, threat map, colori o timing non devono consentire inferenza.

Questa decisione deve riallineare eventuali vecchi testi che parlavano di Overwatch "Confermato/Previsto" sulla base di planning nemico osservato senza un vero evento di reveal.

---

# 16. REVEAL DI REACTION/OVERWATCH

## LOCKED

Esiste un sistema a livelli.

Baseline concettuale:

- `RevealLevel 0 = Hidden`
- `RevealLevel 1 = Presence`
- `RevealLevel 2 = Type`
- `RevealLevel 3 = Direction/Area`
- `RevealLevel 4 = Full Tactical Reveal`

Una ability/sensore/personaggio decide quale livello ottiene.

Il reveal:

- è server-authoritative;
- è un evento;
- entra nel `TurnLog`;
- replica solo i campi autorizzati;
- non espone l'intento canonico.

## 16.1 Snapshot, non tracking live

Default:

`RevealTrackingPolicy = Snapshot`

Se il nemico modifica il proprio planning dopo essere stato rivelato:

- il vecchio reveal **non si aggiorna automaticamente**;
- UI lo può marcare `PotentiallyStale`.

Solo abilità forti:

`RevealTrackingPolicy = Continuous`

## 16.2 Intel condivisa con la squadra

## LOCKED

Il reveal ottenuto da un membro diventa **Team Intel**.

Tutta la squadra riceve lo stesso snapshot sanitizzato.

## 16.3 Durata intel

- intel su intent/reaction pianificata => scade a fine turno;
- intel su stato persistente => resta finché lo stato resta valido o viene esplicitamente invalidato/nascosto.

## 16.4 Last Known State

Per oggetti persistenti rivelati:

- se si spostano/cambiano senza essere osservati, non vengono tracciati magicamente;
- resta `LastKnownState`;
- il marker diventa stale;
- tracking live solo tramite capacità esplicita.

---

# 17. VAULT / LOW WALL

## LOCKED

`LowWall` standard:

- normale Move = Block;
- può essere attraversato tramite traversal speciale `Vault`;
- LOS/cover secondo profilo.

Il Vault standard è una **special transition della fase Move**, non una nuova fase.

Può avere costo maggiore.

Esempio:

`TransitionType = Vault`  
`MovementCost = Base + VaultCost`

Solo ostacoli specifici richiedono `SpecialAbility`.

## 17.1 Numero massimo Vault

Definito dal profilo unità.

Baseline iniziale:

`StandardUnit.MaxVaultsPerMove = 1`

Agile unit può averne 2/unlimited.

Costo e limite sono vincoli separati.

## 17.2 Facing

Baseline:

`StandardVault.FacingPolicy = Preserve`

Profili speciali possono usare:

- `MovementDirection`;
- `ChooseAtLanding`.

## 17.3 Reaction

Vault standard:

`ReactionPolicy = NormalMovement`

Può triggerare Overwatch/reaction di movimento.

Niente posizione intermedia sull'ostacolo.

## 17.4 PreTransition / PostTransition

Le reaction su Vault valutano:

- stato pre-transition, oppure
- stato post-transition,

secondo trigger policy.

Mai una posizione "sopra il muro".

## 17.5 Destinazione occupata

Vault standard:

`OccupiedTargetPolicy = Block`

Push/Swap solo tramite profili/ability speciali.

## 17.6 Fallimento

Se il Vault fallisce:

- unità resta nel SourceHex;
- niente landing alternativa automatica;
- niente re-path opportunistico;
- `FallbackPolicy = StayAtSource` baseline;
- fallback alternative solo pre-dichiarate/pre-validate.

## 17.7 Percorso residuo

Se una transition del Move fallisce:

- Move standard termina;
- path residuo cancellato;
- fallback path solo se pre-pianificata.

## 17.8 Costo fallimento

Default:

`FailureCostPolicy = NoCost`

Il traversal non completato non consuma budget.

## 17.9 MaxVault e fallimento

Default:

`FailedTraversalCountsTowardLimit = false`

Un Vault fallito non consuma il limite.

---

# 18. HAZARD SU CELLA E TRANSITION

## LOCKED

Distinguere:

`CellHazard`
- OnEnter
- OnOccupy
- OnExit

`TransitionHazard`
- OnTraverse

Il Vault standard non bypassa automaticamente hazard.

Ogni hazard può filtrare per `TraversalType`.

Esempi:

- pressure plate può ignorare Vault;
- laser tripwire può triggerare Walk/Vault/Dash;
- barbed wire può triggerare Walk/Vault.

## 18.1 Timing

Transition hazard può essere:

- `PreEntry`;
- `PostEntry`.

`PreEntry` può annullare traversal e lasciare unità nel SourceHex.

`PostEntry` avviene dopo commit nel TargetHex.

Nessuna posizione intermedia.

---

# 19. ORDINE REACTION / HAZARD SU TRANSITION

## LOCKED

Struttura concettuale baseline:

1. Validate Transition
2. PreTraversal
3. PreEntry Hazard
4. Transition Reaction
5. Commit Source->Target
6. OnEnter Reaction / Overwatch
7. PostEntry Hazard

L'ordine per ranked è canonico/deterministico.

All'interno dello stesso stage usare ordering stabile.

---

# 20. TRIGGER GIÀ ACQUISITI NON SI CANCELLANO RETROATTIVAMENTE

## LOCKED

Se l'ingresso in una cella è già stato committato:

- `UnitEnteredCell` è un fatto;
- reaction successiva può Root/Stun/KO;
- PostEntry hazard già generato resta in coda;
- il singolo effetto rivalida la propria `TargetValidityPolicy`.

Esempi policy:

- LivingOnly;
- AliveOrKO;
- AnyPresentUnit;
- Custom.

---

# 21. STATUS CHE INTERROMPONO MOVE

## LOCKED

Status/effect dichiarano una:

`MovementInterruptionPolicy`

Esempi:

- Root => StopImmediately
- Stun => StopImmediately
- Knockdown => StopImmediately
- Slow => Continue + cost modifier
- Marked => None

Se Move viene interrotto:

- percorso residuo cancellato;
- budget residuo del Move perso;
- rimuovere Root in seguito non fa riprendere il vecchio Move;
- un'ability può concedere un **nuovo** movimento con nuovo budget.

---

# 22. MOVE END SEMANTICS

## LOCKED

`MoveEnd` viene sempre generato quando il movimento termina, anche prematuramente.

Conservare:

`ActualMoveEndCell`

`MoveEndReason` almeno:
- Completed
- Blocked
- Interrupted
- Cancelled

`Blocked` = causa spaziale/topologica.

Esempi:
- OccupiedTarget
- TransitionDisabled
- DoorClosed

`Interrupted` = effect/status/reaction.

Esempi:
- RootApplied
- KnockdownApplied
- StunApplied

## 22.1 Primary + Contributing Causes

`MoveEndSummary` conserva:

- `PrimaryCause`;
- `ContributingCauses[]`.

La PrimaryCause è **la prima causa che realmente ferma il Move nell'ordine canonico**.

Non usare ranking artificiale di gravità.

Le contributing causes:

- includono le altre cause dello stesso micro-step/stage che avrebbero impedito la prosecuzione;
- non cercano cause future;
- mantengono ordine canonico;
- applicano deduplica deterministica solo per cause semanticamente identiche.

## 22.2 Dedup semantic identity

CauseType uguale ma source diverse => cause distinte.

Chiave concettuale:

- CauseType
- SourceEntityId
- SourceEffectInstanceId
- TargetEntityId
- TriggerStage

---

# 23. SOURCE / ORIGIN / RESPONSIBILITY

## LOCKED

Il summary di una causa può conservare:

- `OriginSource`
- `ImmediateSource`
- `SourceEffect`

Esempio:

Vektor -> Console -> Turret -> Root

Summary:
- OriginSource = Vektor
- ImmediateSource = Turret
- SourceEffect = RootShot

La catena completa vive nel `TurnLog`.

---

# 24. TURNLOG CAUSAL GRAPH

## LOCKED

Ogni evento può avere:

`CauseEventIds[]`

ordinato deterministicamente.

La causalità è un **DAG**, non necessariamente una catena singola.

Nessun `PrimaryCauseEventId` generale necessario: la causa primaria rimane concetto dei summary specifici.

## 24.1 No cycles

Cicli vietati.

Validator/debug:

- cause exists;
- cause precedes event;
- no causal cycles;
- stable cause ordering.

## 24.2 EventId match-global

Gli `EventId` sono univoci per l'intero match e possono attraversare confini di turno.

Una mina piazzata al Turn 4 può essere causa di un evento al Turn 5.

---

# 25. PROVENANCE DEGLI OGGETTI PERSISTENTI

## LOCKED

Oggetto/effect persistente conserva:

- `OriginEventId` autorevole;
- dati denormalizzati utili:
  - `OriginEntityId`
  - `OriginTeamId`

Non copia l'intera causal chain.

La storia completa si ricostruisce via `TurnLog`.

---

# 26. OWNERSHIP CAMBIABILE

## LOCKED

Origin e current owner restano distinti.

Esempio turret:

- OriginEntityId = Vektor
- OriginTeamId = TeamA
- CurrentOwnerEntityId = Riva
- CurrentTeamId = TeamB
- LastOwnershipChangeEventId = ...

Ogni hack/cambio ownership genera evento causale.

La storia completa ownership vive nel TurnLog, non in una lista duplicata sull'oggetto.

---

# 27. GAMEPLAY CREDIT

## LOCKED

Per effetti futuri di un oggetto controllabile:

baseline `CreditPolicy = CurrentController`.

Conservare distinti:

- Origin;
- ImmediateSource;
- ResponsibleEntity/Team.

Profili speciali possono usare:

- CurrentController
- OriginalCreator
- ImmediateSourceOnly
- Shared
- NoCredit

## 27.1 Delayed effect

Il credito di un effetto ritardato viene snapshotato **quando l'effetto viene creato**.

Cambio owner successivo non riscrive retroattivamente responsabilità di missile/proiettile/effect già creato.

`CreditSnapshotPolicy = OnEffectCreation` baseline.

## 27.2 Contributors

Supportare:

- `PrimaryResponsibleEntity`
- `Contributors[]`

Non usare pesi percentuali.

Usare ruoli discreti:

- Modifier
- Amplifier
- Enabler
- Trigger
- Controller

Una entità compare una volta e può avere più Roles.

Baseline contribution propagation:

`DirectOnly`

Propagazione a enabler indiretti solo se policy esplicita.

---

# 28. LAYER, PAVIMENTI E SOFFITTI

## LOCKED

Due celle con stesso X/Y e Layer diversi NON sono automaticamente connesse.

Pavimento/soffitto integro:

- Movement Block;
- LOS Block;
- Projectile Block.

Connessioni verticali devono essere esplicite:

- Stairs
- Ramp
- Ladder
- Elevator
- Hole
- DestroyedFloor
- Shaft
- Skylight
- Climb
- VaultUp
- JumpDown

Skylight può ad esempio:

- Movement Block;
- LOS Pass;
- Projectile per profilo.

---

# 29. CADUTA FORZATA DA PAVIMENTO DISTRUTTO

## LOCKED

Se il pavimento sotto una unità viene distrutto:

- generare `ForcedFall`;
- destinazione determinata dal grafo/logica verticale;
- niente rigid-body authority;
- possibile FallDamage / Knockdown;
- trigger OnEnter sulla landing;
- può concatenarsi su più Layer se mancano altri pavimenti.

---

# 30. DROP / JUMPDOWN VOLONTARIO

## LOCKED

Consentito tramite traversal esplicito:

`Drop/JumpDown`

Non è normale Move attraverso il vuoto.

Può avere:

- limite altezza;
- costo;
- danno;
- Knockdown;
- reaction;
- landing cell valida obbligatoria.

Personaggi agili possono avere profili migliori.

---

# 31. SALITA VERTICALE

## LOCKED

Nessuna salita automatica perché il Layer superiore è "vicino".

Serve:

- transition esplicita Climb/VaultUp/Ladder/Stairs/Ramp/Elevator;
- oppure ability Grapple/JetJump/WallClimb/Teleport.

A* considera solo transition compatibili con unit profile.

---

# 32. PUSH / KNOCKBACK E BORDI

## LOCKED

Forced Movement può spingere una unità oltre un bordo e trasformarsi in `ForcedFall`, se esiste una relazione di drop valida.

Niente "salvataggio automatico" universale.

Possibili difese esplicite:

- Brace;
- Anchored;
- anti-displacement;
- LedgeGrab;
- GrappleSave;
- AllyIntercept.

---

# 33. LETHAL DROP / OUT OF BOUNDS

## LOCKED

Se non esiste landing cell valida, il profilo del bordo decide.

Esempio:

`DropProfile =`
- SafeLanding
- DamagingFall
- LethalDrop
- OutOfBounds

`LethalDrop` può produrre KO immediato.

`OutOfBounds` produce esito definito dal ruleset.

Il bordo pericoloso deve essere leggibile se informazione pubblica.

---

# 34. REACTION DI SALVATAGGIO DA PRECIPIZIO

## LOCKED

Nessuna protezione automatica per tutti.

Solo reaction/ability esplicite.

Flow concettuale:

ForcedMovement  
-> LethalDrop detected  
-> legal reaction opportunity  
-> resolve reaction  
-> se fallisce/nessuna reaction => ForcedFall/KO

Integrare nel framework reaction esistente.

---

# 35. CADUTA SU CELLA OCCUPATA

## LOCKED

Una `ForcedFall` può impattare una cella occupata e generare:

`FallCollision`

Possibili effetti:

- damage falling unit;
- damage occupant;
- Knockdown;
- ForcedDisplacement occupant.

Mai due unità nello stesso hex.

MomentumDirection del Push può guidare la direzione preferita di displacement.

Caduta verticale pura usa una preferenza deterministica definita dal profilo.

---

# 36. FORCED DISPLACEMENT A CATENA

## LOCKED

Consentire catene:

A cade su B  
B->C  
C->D  
D->E libero

Se la catena può essere risolta, commit atomico dal fondo.

Se termina contro blocker/non-movable/ciclo:

- niente stacking;
- niente spostamenti parziali arbitrari;
- si applica collisione bloccata/Impact secondo profilo.

---

# 37. BLOCKED DISPLACEMENT -> IMPACT

## LOCKED

Il displacement residuo può trasformarsi in `Impact`.

Categorie discrete:

`ImpactStrength = None | Light | Medium | Heavy`

Possibili outcome:

- ImpactDamage;
- Stagger;
- Knockdown;
- StructuralDamage.

Non usare velocità fisiche real-time.

---

# 38. FORCED MOVEMENT E OSTACOLI ATTRAVERSABILI

## LOCKED

Forced Movement NON eredita automaticamente il traversal volontario.

Ogni ostacolo dichiara policy separata.

Esempio:

`LowWall`
- NormalMove = Vault
- ForcedMovement = Stop

`LowBarrier`
- ForcedMovement = ForceOverAllowed

Una ability potente può avere `CanForceOver`.

---

# 39. KNOCKBACK E OSTACOLI DISTRUTTIBILI

## LOCKED

Un Knockback abbastanza forte può:

1. impattare ostacolo;
2. applicare StructuralImpact;
3. romperlo/breach;
4. continuare oltre se il nuovo stato permette ForcedMovement.

Confronto discreto:

- `ImpactStrength`
- `StructuralResistance`

Niente mesh physics come decision maker.

## 39.1 BreakAndContinue same event

La breccia creata dal Knockback può essere utilizzata **immediatamente dallo stesso evento**.

Flow:

Push  
-> Impact  
-> BreachSlot Intact->Breached  
-> update logical map  
-> verify new ForcedMovementPolicy  
-> continue displacement

Eventi successivi nello stesso resolver vedono già la breccia.

Eventi già risolti non si ricalcolano.

---

# 40. BREACH SLOT

## LOCKED

Brecce runtime non sono fori continui arbitrari.

Usare `BreachSlot` predefiniti.

Targeting può sembrare spaziale, ma il resolver snap/risolve deterministicamente allo slot.

Ogni slot definisce effetti pre-baked su:

- ControlledCells;
- ControlledTransitions;
- LOS;
- Cover.

## 40.1 Integrity

BreachSlot può avere Integrity persistente.

Efficacia dipende dal damage profile:

- rifle basso;
- explosion alto;
- demolition alto.

Niente "65% di buco" sulla base dell'HP.

## 40.2 Stati

Framework supporta stati:

- Intact
- Damaged
- Critical
- Breached
- Patched/Reinforced

Baseline standard può essere solo Intact->Breached.

Qualsiasi effetto tattico intermedio deve essere esplicito.

## 40.3 Slot vicini

Indipendenti di default.

AoE può danneggiarne più di uno.

Propagazione strutturale solo se profilo lo dichiara.

## 40.4 Riparazione

Riparare breach non significa necessariamente tornare Intact.

Può diventare:

- Patched;
- Reinforced;
- nuova barricade object.

---

# 41. COVER DA BREACH

## LOCKED

Breach standard:

- non dà cover automaticamente;
- se LOS passa nel varco, cover = None salvo profilo.

`BreachProfile` può dichiarare Light/Heavy.

Non derivare cover dalla larghezza o dai "pezzi di muro rimasti ai lati".

---

# 42. STRUCTURAL DEPENDENCIES E CROLLI A CATENA

## LOCKED

Supportare crolli a catena, ma solo tramite dipendenze logiche esplicite.

Esempio:

Support_03  
-> FloorSection_07  
-> WallSection_12

Ogni elemento può avere:

`OnDependencyLost =`
- None
- Damaged
- Critical
- Collapse
- DelayedCollapse

Chaos non decide cosa crolla.

Il grafo strutturale deve essere:

- validato;
- deterministico;
- senza cicli;
- con limiti contro cascata patologica.

---

# 43. CROLLO IMMEDIATO E DELAYED/TELEGRAPHED

## LOCKED

Supportare entrambi.

Possibili stati:

Stable  
-> Damaged  
-> Critical  
-> CollapsePending  
-> Collapsed

`CollapsePending` è informazione pubblica.

Il timing può essere:

- EndOfStage;
- EndOfTurn;
- after N turns;

mai legato all'animazione.

Un nuovo danno può anticipare il crollo solo se il profilo lo prevede.

---

# 44. RISULTATO DEL CROLLO: MACERIE / NUOVO TERRENO

## LOCKED

Il crollo può trasformare la mappa creando:

- Empty
- Rubble
- HeavyRubble
- DebrisField
- BlockedDebris
- Custom

Il nuovo terreno può modificare:

- MovementPolicy;
- MovementCost;
- Cover;
- LOS;
- ProjectilePolicy;
- Hazard;
- VerticalTransitions;
- GraphRevision.

Il risultato è stato logico persistente, non mesh casuale.

---

# 45. MACERIE ULTERIORMENTE MODIFICABILI

## LOCKED — decisione finale corretta

Dopo un iniziale "No", la decisione finale è stata **Sì**.

Le macerie/terrain post-distruzione possono essere ulteriormente trasformati tramite transizioni discrete.

Esempi:

IntactWall  
-> HeavyRubble  
-> Rubble  
-> Clear

oppure:

Rubble  
-> BurningDebris  
-> Ash

Usare state machine esplicita. Niente quantità continue.

---

# 46. DISTRUZIONE E CONNETTIVITÀ DELLA MAPPA

## LOCKED

La distruzione può:

- chiudere completamente una rotta;
- aprirne una nuova;
- rendere una zona irraggiungibile;
- spezzare collegamenti tra Layer.

Esempi:

- bridge collapsed;
- tunnel blocked;
- elevator disabled;
- wall breached.

Non mantenere path artificiali per comodità.

## 46.1 No soft-lock involontari

Le mappe devono essere validate contro soft-lock del match non desiderati.

Gli objective critici devono:

- restare completabili nei structural states previsti;
- oppure dichiarare esplicitamente che la loro perdita/isolamento è una condizione valida di vittoria/sconfitta.

---

# 47. ISOLAMENTO INTENZIONALE DI UNITÀ

## LOCKED

È gameplay valido.

Distruggere ponte/tunnel/passaggio può isolare una unità.

Non fornire:

- teleport rescue automatico;
- path artificiale;
- escape gratuito.

La unità continua a giocare nella propria componente del grafo.

Ability come Grapple/JetJump/Teleport/BridgeDeploy possono offrire soluzioni.

Il match nel suo complesso deve però restare risolvibile secondo il ruleset.

---

# 48. STRUCTURAL POLICY

## LOCKED

Ogni elemento strutturale dichiara esplicitamente come può essere distrutto.

Baseline:

`StructuralPolicy =`
- Indestructible
- Destructible
- BreachableOnly
- CollapseOnly
- Scripted

Semantica:

- Indestructible => structural damage non modifica stato;
- Destructible => usa state machine del profilo;
- BreachableOnly => solo BreachSlot autorizzati;
- CollapseOnly => cambia per dependency/collapse;
- Scripted => solo objective/scenario/interazione esplicita.

---

# 49. ACQUA — PROFONDITÀ TATTICA

## LOCKED

Acqua con classi discrete:

`WaterDepth =`
- None
- Shallow
- Deep
- Impassable

## Shallow
- occupabile da unità standard;
- costo movimento modificato;
- Wet secondo profilo;
- conduzione elettrica.

## Deep
- bloccata per unità standard;
- consentita a profili espliciti:
  - Swim
  - Amphibious
  - Hover
  - WaterWalk
  - ecc.

## Impassable
- non occupabile da unità normali;
- per canali/bacini/bordi ecc.

Anche nuotando si occupa sempre un `FRTCellId`.

---

# 50. ACQUA DINAMICA / FLOODING

## LOCKED

La profondità può cambiare durante il match tramite stati discreti e propagazione deterministica.

Esempio:

Dry -> Shallow -> Deep -> Impassable

Può:

- rallentare;
- aprire/chiudere rotta;
- spegnere fuoco;
- creare Wet;
- cambiare rete elettrica;
- interagire col rumore;
- favorire profili anfibi.

Ogni cambio aggiorna:

- MapState;
- path costs/validity;
- hazard;
- GraphRevision.

Niente livello visuale dell'acqua come autorità.

---

# 51. UNITÀ SU CELLA CHE DIVENTA TROPPO PROFONDA

## LOCKED

Se una cella cambia depth e l'occupante non è compatibile:

1. WaterDepth changes;
2. check occupant compatibility;
3. se compatibile => resta;
4. se incompatibile => tentare `ForcedEscape`;
5. se nessuna destinazione valida => applicare hazard di acqua profonda.

Possibili conseguenze:

- Drowning;
- Damage;
- Immobilized;
- Custom.

Non KO automatico universale.

`ForcedEscape`:
- non consuma Move Budget;
- può interrompere Move;
- non attraversa unità;
- niente stacking;
- rispetta muri/Layer/transitions;
- usa destinazioni legalmente occupabili;
- deterministic.

---

# 52. CORRENTE D'ACQUA

## LOCKED

Supportare flow discreto.

Esempio:

`WaterFlowDirection = HexDirection`

`WaterFlowStrength =`
- None
- Weak
- Strong

Corrente può produrre `ForcedMovement`.

Può:

- spostare unità;
- spostare oggetti compatibili;
- causare collisioni;
- concatenare displacement;
- trascinare in hazard;
- causare ForcedFall se porta verso bordo/drop.

Resistenze:

- Anchored;
- weight/profile;
- Swim/Amphibious;
- ability.

Niente fluid dynamics real-time.

---

# 53. ELETTRICITÀ NELL'ACQUA — ULTIMA DECISIONE PRIMA DELLA PAUSA

## LOCKED — RESUME MARKER

**L'elettricità nell'acqua si propaga attraverso la CONNETTIVITÀ LOGICA DELLA RETE D'ACQUA, non tramite semplice raggio geometrico.**

Quindi:

ElectricSource  
-> ConnectedWaterGraph  
-> deterministic propagation  
-> ElectrifiedWater states

Questo deve consentire:

- canali elettrificati;
- acqua isolata non raggiunta;
- porte/chiuse che spezzano o riconnettono la rete;
- profondità/profili che possono modificare la conduzione;
- combo acqua + elettricità;
- stato logico deterministico.

**PAUSA DELLA CONVERSAZIONE QUI.**

Quando si riprende il focus design, continuare da questo punto.

Prossimi nodi plausibili da discutere SOLO se realmente importanti:
- attenuazione/intensità della propagazione elettrica sulla water network;
- durata dello stato ElectrifiedWater;
- effetto di depth/flow/materiale sulla conduzione;
- interazione con gate/valve/opening;
- ordine tra flooding, flow, elettricità e unità nello stesso micro-step;
- eventuale chain verso metallo/oggetti conduttivi;
- counterplay e telegraphing.

Non riaprire le decisioni sopra senza una ragione concreta.

---

# 54. REGOLE GENERALI DI IMPLEMENTAZIONE DERIVATE

## 54.1 Dati discreti, non fisica come autorità

Tutto ciò che riguarda:
- movimento;
- cover;
- LOS;
- breach;
- cadute;
- corrente;
- collisioni;
- crolli;
- acqua;
- conduzione;

deve essere espresso in:
- celle;
- transition;
- state machine;
- profili;
- ID stabili;
- ordering deterministico.

Chaos/ragdoll/VFX/mesh = presentation.

## 54.2 Separazione servizi

Mantenere separati:
- Pathfinding
- LOS
- Targeting
- Trajectory
- Structural
- Water/Environment
- Reaction
- Intel/Perception

Non riusare una `ControlledTransition` come scorciatoia per LOS/cover.

## 54.3 GraphRevision / invalidazione cache

Qualsiasi modifica che cambia:
- traversability;
- transition;
- cost;
- blocker;
- bridge;
- door;
- breach;
- flood depth;
- collapse;
- structural result;

deve aggiornare revisioni appropriate e invalidare cache di path/preview.

## 54.4 Determinismo

Mai dipendere da:
- Tick order;
- TMap/TSet iteration;
- Actor order;
- packet arrival;
- animation timing;
- physics resolution;
- floating-point geometry ambiguities non quantizzate.

Usare:
- stable IDs;
- canonical stage order;
- integer/fixed-point dove serve;
- reason code;
- golden tests.

## 54.5 Networking/privacy

Server-only:
- CanonicalIntentStore;
- planning completo;
- hidden reaction;
- hidden Overwatch.

Team-only:
- ally intent;
- TeamIntel sanitizzata;
- reveal autorizzato.

Public:
- stato ambientale già pubblico;
- collapse pending pubblico;
- structural result risolto;
- risultati autoritativi.

Non replicare planning nemico e poi "nasconderlo in UI".

---

# 55. UI / DEBUG DA AGGIORNARE

Aggiornare la Wiki e Feature Map includendo overlay/debug per:

## Geometry Debug
- cell center;
- wall segments;
- aperture;
- controlled cells;
- controlled transitions;
- breach slots;
- structural dependencies.

## Cover Debug
Per cella:
- 6 cover sectors;
- source geometry/profile;
- override marker;
- elevation degradation.

## LOS Debug
Mostrare:
- SourceCell;
- TargetCell;
- logical line;
- blocker;
- opening;
- occlusion class;
- elevation result;
- reason code.

## Traversal Debug
Mostrare:
- Normal;
- Vault;
- JumpDown;
- Climb;
- ForcedMovement;
- vertical transition;
- blocked reason.

## Collision Debug
Mostrare per micro-step:
- proposals;
- dependency graph;
- contested destination;
- chain success/failure;
- commit set.

## Structural Debug
- StructuralPolicy;
- Integrity;
- BreachSlot state;
- dependencies;
- CollapsePending;
- CollapseResult.

## Water Debug
- WaterDepth;
- water connectivity;
- flow direction;
- flow strength;
- Electrified state/network.

## Intel Debug
Server/debug-only:
- hidden intent;
- reveal level;
- TeamIntel payload;
- snapshot/continuous policy;
- stale/last-known status.

Importantissimo: i debug server-only non devono finire accidentalmente nei client avversari in build competitive.

---

# 56. AUTOMATION / FUNCTIONAL TESTS DA AGGIUNGERE

Claude deve creare/aggiornare backlog e test plan almeno per:

## Geometry / Opening
- wall through cell center => Blocked;
- wall inside hex but not center => walkable + transition blocks;
- aperture interior crossing => pass;
- exact boundary => block;
- editor override;
- one door controls N cells/N transitions;
- occupied doorway prevents close.

## Cover
- 6-sector mapping deterministic;
- exact sector tie deterministic;
- one geometry -> multiple sectors;
- strongest cover wins;
- facing independent;
- symmetric default / FrontOnly profile;
- elevation degrades cover;
- high ground no global numeric bonus.

## LOS
- closed/open door;
- boundary conservative;
- full wall + elevation class;
- floor/ceiling separation;
- skylight/hole profiles.

## Traversal
- LowWall normal move blocked;
- Vault allowed;
- max vault per unit;
- failure no cost;
- failure doesn't consume limit;
- occupied target blocks;
- predeclared fallback only;
- reaction on Vault.

## Occupancy / Simultaneous Move
- same destination => all block;
- open-ended chain => commit;
- chain ending in blocker => fail backwards;
- swap => fail;
- closed cycle => fail;
- PreEntry cancellation invalidates upstream.

## MoveEnd / Cause
- correct Blocked vs Interrupted;
- PrimaryCause = first stopping cause;
- contributing causes same micro-step only;
- semantic dedup;
- stable ordering.

## TurnLog causal DAG
- CauseEventIds exists;
- cause precedes event;
- no cycles;
- cross-turn causality;
- stable ordering;
- replay hash stable.

## Ownership/Credit
- hack changes current owner only;
- origin immutable;
- delayed effect snapshots responsibility;
- contributors + roles;
- direct-only propagation baseline.

## Verticality
- layer not auto-adjacent;
- JumpDown;
- Climb;
- floor destruction -> ForcedFall;
- lethal drop;
- reaction save;
- fall onto occupied cell.

## Forced Displacement
- chain displacement;
- blocked chain -> impact;
- force over low obstacle by profile;
- structural break & continue;
- new breach used in same event.

## Structural
- dependency collapse;
- delayed collapse;
- collapse result terrain;
- rubble -> later state transitions;
- structural graph no cycles;
- map connectivity changes;
- soft-lock validator.

## Water
- Shallow/Deep/Impassable;
- flood changes path;
- ForcedEscape;
- no legal escape -> drowning/hazard;
- current ForcedMovement;
- current collision chain;
- water graph connectivity;
- electricity propagates only over connected water graph.

## Network privacy
- hidden Overwatch absent from enemy packets/memory-visible DTOs;
- reveal levels expose exact whitelist only;
- snapshot reveal does not update after enemy edit;
- TeamIntel shared only to correct team;
- stale last-known state;
- canary tests packaged.

---

# 57. FEATURE MAP — FEATURE DA CREARE/AGGIORNARE

Se non esistono già, creare/aggiornare feature entries per:

1. Hex Architectural Geometry
2. Wall/Aperture Bake
3. Doorway Cells & Controlled Transitions
4. Discrete Cover Sectors
5. Elevation-aware Cover
6. Logical Occlusion Heights
7. Special Traversal Profiles
8. Vault
9. Vertical Traversal
10. Simultaneous Occupancy Resolver
11. Forced Movement
12. Forced Fall / Lethal Drop
13. Fall Collision
14. Displacement Chains
15. Structural Impact
16. Breach Slots
17. Structural Dependencies
18. Immediate & Delayed Collapse
19. Collapse Result Terrain
20. Destruction Connectivity Changes
21. Structural Map Validator
22. Water Depth
23. Dynamic Flooding
24. Forced Escape from Water
25. Water Flow / Current
26. Water Connectivity Graph
27. Electrified Water Propagation
28. Team Intel Store
29. Reaction Reveal Levels
30. Snapshot vs Continuous Tracking
31. Hidden Overwatch Privacy
32. Causal TurnLog DAG
33. Ownership/Provenance/Credit
34. MoveEnd Explainability

Ogni feature deve avere:
- status;
- milestone;
- owner se schema lo prevede;
- dependencies;
- wiki link;
- issue/epic link;
- scenario/test link;
- acceptance criteria sintetici.

---

# 58. SCENARIO MAP — SCENARI DA CREARE/AGGIORNARE

Creare scenari piccoli e mirati, non una singola mega-mappa.

Suggeriti:

## SCN-GEO-01 Wall Through Cell Center
Verifica cell blocked.

## SCN-GEO-02 Wall Crossing Hex Off-Center
Cell walkable, transition selective block.

## SCN-DOOR-01 Multi-Cell Gate
Open/close, occupant blocking close.

## SCN-COVER-01 Six Sector Cover
Attacchi da tutte le sei direzioni.

## SCN-COVER-02 Elevation vs LowWall
Same/+1/+2 elevation.

## SCN-LOS-01 Full Wall Overlook
Source su Layer alto vede/non vede secondo OcclusionHeightClass.

## SCN-MOVE-01 Simultaneous Chain
A->B->C->free.

## SCN-MOVE-02 Contested Destination
Due unità stessa cella.

## SCN-MOVE-03 Swap Rejected
A<->B.

## SCN-VAULT-01 Vault + Overwatch
Reaction pre/post transition.

## SCN-HAZ-01 PreEntry Trap
Blocca traversal.

## SCN-STRUCT-01 Knockback Breach
Nemico sfondato attraverso muro.

## SCN-STRUCT-02 Support Collapse
Distruzione supporto -> floor collapse -> ForcedFall.

## SCN-STRUCT-03 Delayed Collapse
CollapsePending visibile.

## SCN-FALL-01 Lethal Drop
Push -> reaction save/fail -> KO.

## SCN-FALL-02 Fall Collision Chain
A cade su B -> B sposta C -> ecc.

## SCN-WATER-01 Flood Route
Dry->Shallow->Deep modifica A*.

## SCN-WATER-02 Forced Escape
Unità non-swimmer in cell che diventa Deep.

## SCN-WATER-03 Current Push
Corrente produce forced displacement.

## SCN-WATER-04 Connected Electricity
Elettricità percorre solo celle acqua connesse.

## SCN-INTEL-01 Hidden Overwatch
Enemy client non riceve nulla.

## SCN-INTEL-02 Reveal Snapshot
Reveal level 3, enemy cambia plan, snapshot resta stale.

## SCN-INTEL-03 Team Share
Un solo scout rileva, tutta la squadra riceve TeamIntel sanitizzata.

---

# 59. EDITOR MAP — TASK MANUALI/EDITOR DA AGGIUNGERE

Separare ciò che richiede Unreal Editor da ciò che può essere implementato/testato headless.

Task Editor suggeriti:

1. Wall authoring tool / segment placement
2. Geometry junction/corner visualization
3. Aperture placement + margin validator
4. ControlledCells/Transitions visualization
5. Cover sector auto-bake preview
6. Cover override inspector
7. Occlusion height class visualization
8. Layer connection editor
9. Vertical transition placement
10. Drop/LethalDrop authoring
11. BreachSlot placement
12. Structural dependency graph visualization
13. Collapse result terrain preview
14. Water depth painting
15. Water connectivity visualization
16. Water flow direction painting/arrows
17. Electrified water network debug
18. Scenario selector entries per scenario sopra
19. Map validation command/button
20. Debug overlays toggles

Per ogni task Editor Map:
- descrivere input manuale richiesto;
- asset/mappa interessata;
- screenshot/debug proof richiesto;
- link a issue;
- DoD Editor.

---

# 60. ROADMAP — POSIZIONAMENTO RACCOMANDATO

Non inventare nuove milestone se esistono già equivalenti; riallineare la roadmap reale.

Suggerimento coerente con i PDR attuali:

## F0 Fondazioni
Solo subset minimo:
- cell occupancy;
- simultaneous move collision;
- move end reasons;
- basic TurnLog;
- first A*;
- graybox.

## F1 Networking Privacy
- hidden intent contract;
- team-only relay;
- privacy canary;
- foundation TeamIntel transport interfaces senza gameplay completo.

## F2 Abilities/Reaction
- Vault profile where needed;
- reaction framework;
- Overwatch secrecy;
- reveal plumbing;
- forced movement base.

## F3 Mappa Multilivello
Epic principale per:
- wall geometry;
- openings;
- cover sectors;
- LOS elevation;
- vertical transitions;
- structural;
- flooding;
- current;
- water electricity;
- collapse;
- map validators.

## F4 Vertical Slice
- scenari completi;
- character abilities usando i sistemi;
- UI threat/intel;
- environmental combo;
- polish/readability.

## F5 Dedicated/Replay Audit
- causal DAG persistence;
- privacy packaged audit;
- replay determinism;
- scale/performance.

Se la roadmap reale ha già versioni più avanzate, NON retrocedere il lavoro: mappare le feature nelle milestone reali.

---

# 61. EPIC / ISSUE — AZIONE RICHIESTA A CLAUDE

Claude deve ispezionare GitHub/repository e poi:

## EPIC A — Tactical Map Geometry & Cover
Include:
- wall authoring/bake;
- aperture;
- controlled cells/transitions;
- 6-sector cover;
- elevation response;
- logical occlusion;
- editor debug/validator.

## EPIC B — Traversal, Occupancy & Simultaneous Movement
Include:
- occupancy blocker;
- simultaneous proposals;
- dependency chains;
- contested destination;
- swap/cycle rejection;
- Vault;
- vertical traversal;
- hazard timing.

## EPIC C — Forced Movement & Vertical Hazards
Include:
- Push/Knockback;
- ForcedFall;
- LethalDrop;
- save reactions;
- FallCollision;
- displacement chains;
- blocked impact.

## EPIC D — Structural Destruction & Collapse
Include:
- StructuralPolicy;
- BreachSlot;
- integrity;
- break-and-continue;
- dependency graph;
- immediate/delayed collapse;
- rubble/result terrain;
- connectivity validator.

## EPIC E — Water Simulation (Discrete)
Include:
- WaterDepth;
- flooding;
- ForcedEscape;
- current;
- flow direction;
- connected water graph;
- ElectrifiedWater propagation.

## EPIC F — Reaction Intel & Privacy
Include:
- hidden Overwatch;
- reveal levels;
- snapshot tracking;
- TeamIntel;
- stale/last-known;
- anti-leak network tests.

## EPIC G — TurnLog Causality & Explainability
Include:
- CauseEventIds DAG;
- match-global EventIds;
- MoveEnd primary/contributing causes;
- provenance;
- ownership;
- credit/contributor roles;
- replay explanation.

Per ogni epic/issue:

- controllare se esiste già;
- aggiornare titolo/body/labels/milestone invece di duplicare;
- aggiungere acceptance criteria;
- aggiungere test automatici;
- aggiungere privacy impact;
- aggiungere determinism impact;
- linkare Wiki/Feature/Scenario/Editor map;
- inserire dependency links;
- usare ID/label convenzioni del repository.

---

# 62. DOCUMENTAZIONE/WIKI — PAGINE DA AGGIORNARE

Aggiornare o creare pagine dedicate almeno per:

- Tactical Hex Geometry
- Walls and Openings
- Doors and Doorway Cells
- Cover Model
- LOS and Elevation
- Traversal Profiles
- Vault
- Simultaneous Movement and Occupancy
- Forced Movement
- Falls and Lethal Drops
- Structural Destruction
- Breach Slots
- Structural Collapse
- Water Depth and Flooding
- Water Current
- Electrified Water
- Overwatch Secrecy
- Team Intel and Reveal
- TurnLog Causality
- Provenance and Gameplay Credit

Ogni pagina deve separare:
- regola di gameplay;
- data model concettuale;
- resolver timing;
- networking/privacy;
- UI/debug;
- test;
- future extensions.

---

# 63. VALIDATOR DA AGGIUNGERE

Aggiungere/aggiornare validator per:

## Geometry
- duplicate/invalid geometry IDs;
- unsupported junction;
- opening touching prohibited corner;
- ambiguous boundary;
- invalid override.

## Cover
- invalid sector;
- cover profile unknown;
- impossible layer/elevation mapping;
- duplicate conflicting override.

## Graph
- transition to missing cell;
- unsupported vertical relation;
- structural change leaves dangling edge;
- invalid forced movement relation.

## Structural
- structural dependency cycle;
- missing collapse result;
- breach slot outside parent structure;
- inconsistent StructuralPolicy;
- collapse changes objective into accidental soft-lock.

## Water
- water graph invalid;
- flow without valid direction;
- Deep/Impassable state with incompatible authored transition;
- current into impossible target without policy;
- electrified water connectivity mismatch.

## Intel/privacy
- hidden reaction type in public/team-wrong DTO;
- TeamIntel payload contains non-whitelisted field;
- continuous tracking without explicit policy.

---

# 64. BALANCE MATRIX — AGGIORNAMENTI RICHIESTI

Aggiornare `RefactorTactics_Balance_Matrices_v0.1.xlsx` o la sua sorgente dati, preservando lo schema reale, con tabelle/righe per:

## Traversal
- VaultCost
- MaxVaultsPerMove
- ClimbCost
- JumpDownCost
- fall category

## Structural
- ImpactStrength
- StructuralResistance
- Integrity
- CollapseDelay
- CollapseResult

## Water
- WaterDepth class
- movement cost
- compatible movement profiles
- flow strength
- flow displacement
- electrification duration/intensity quando definita

## Intel
- reveal level
- duration
- tracking policy
- source ability
- counterplay

Non inventare valori finali se non sono stati decisi: usare placeholder `TBD` con owner/issue.

---

# 65. NETWORK SECURITY CHECKLIST

Ogni implementazione che tocca reaction/intel deve dimostrare:

- enemy client non riceve hidden Overwatch;
- nessuna proprietà replicated globale contiene planning;
- nessun public TurnLog anticipato contiene intenti;
- nessun path/threat preview usa hidden enemy data;
- packet frequency/size non rivela cambi di piano sensibili se evitabile;
- TeamIntel è whitelist DTO;
- reconnect/late join non ricostruisce intel non autorizzata;
- spectator/replay policy esplicita;
- packaged canary test.

---

# 66. DEFINITION OF DONE PER QUESTO BLOCCO

Una feature non è Done se manca uno dei seguenti:

1. server-authoritative logic;
2. determinismo testato;
3. nessun leak privacy;
4. reason code / TurnLog;
5. debug overlay pertinente;
6. Automation/Functional Test;
7. packaged verification;
8. documentazione/Wiki;
9. Feature Map link;
10. Scenario Map link;
11. Editor Map entry se richiede authoring manuale;
12. issue/epic collegata;
13. performance instrumentation se tocca path/LOS/resolver.

---

# 67. COMMIT / PR STRATEGY RACCOMANDATA

Non fare un mega-commit.

Se il lavoro è solo di consolidamento documentale/backlog, usare commit focalizzati, ad esempio:

1. `docs(map): consolidate hex geometry cover traversal and verticality`
2. `docs(structure): document breach collapse and forced movement rules`
3. `docs(environment): consolidate water depth flow and electrification`
4. `docs(intel): consolidate hidden overwatch reveal and team intel`
5. `chore(roadmap): align feature scenario editor maps and backlog`

Se vengono creati/aggiornati issue GitHub, nel report finale elencare:
- epic;
- issue;
- milestone;
- link;
- created/updated;
- duplicate avoided/merged.

---

# 68. OUTPUT RICHIESTO A CLAUDE AL TERMINE

Claude deve restituire un report con:

## Files changed
Elenco path reali.

## Wiki
Pagine create/aggiornate.

## Roadmap
Milestone/entries aggiornate.

## Feature Map
Feature ID + status + issue link.

## Scenario Map
Scenario ID + purpose + linked tests.

## Editor Map
Task manuali + linked issue.

## GitHub
Epic/issues:
- existing updated;
- new created;
- duplicates avoided.

## Conflicts resolved
Vecchie specifiche superate e come sono state corrette.

## Tests/backlog
Test creati oppure issue create per implementarli.

## Resume point
Confermare che il prossimo focus di design riparte da:

> **Elettricità nell'acqua: propagazione lungo il Connected Water Graph.**

---

# 69. NON FARE

- Non reintrodurre muri sui lati degli esagoni.
- Non introdurre sub-cell movement.
- Non far attraversare alleati di default.
- Non introdurre ZoC universale.
- Non rendere Overwatch nemico visibile senza reveal.
- Non usare mesh height per decidere cover/LOS.
- Non usare Chaos/physics come autorità.
- Non scegliere collision winner tramite initiative implicita.
- Non riusare transition movement come LOS.
- Non fare re-path opportunistico durante resolution.
- Non copiare l'intera causal history dentro ogni oggetto persistente.
- Non permettere cicli nel causal DAG o structural dependency graph.
- Non generare soft-lock non intenzionali.
- Non trattare la distruzione come puramente cosmetica.
- Non usare raggio geometrico come modello base per elettricità in acqua.

---

# 70. RESUME MARKER

**Conversazione messa in pausa subito dopo questa decisione:**

> L'elettricità nell'acqua si propaga attraverso la connettività logica della rete d'acqua, non tramite semplice distanza/raggio.

Quando riprendiamo:
- non ripetere il blocco precedente;
- proseguire da conduzione elettrica su water network;
- porre solo decisioni di design realmente importanti;
- scegliere autonomamente i dettagli minori coerenti con le decisioni già consolidate.


---

# APPENDICE — CHECK FINALE PER CLAUDE

Prima di chiudere il lavoro, Claude deve verificare che il repository documenti esplicitamente TUTTI questi macro-blocchi:

- [ ] Hex grid fundamentals
- [ ] Geometry directions / 90° architecture
- [ ] Cell center geometry references
- [ ] Wall vs cell blocking
- [ ] Wall vs transition blocking
- [ ] Door/opening binding
- [ ] Doorway cells
- [ ] Window policies
- [ ] Breach slots
- [ ] 6-sector cover
- [ ] elevation-aware cover
- [ ] logical LOS heights
- [ ] LowWall/Vault
- [ ] traversal failure/fallback
- [ ] cell/transition hazards
- [ ] simultaneous occupancy
- [ ] contested destinations
- [ ] no ally pass-through
- [ ] no universal ZoC
- [ ] hidden Overwatch
- [ ] reveal levels
- [ ] team intel
- [ ] snapshot/stale intel
- [ ] vertical layers
- [ ] floor/ceiling separation
- [ ] JumpDown/Climb
- [ ] ForcedFall/LethalDrop
- [ ] fall collision
- [ ] displacement chains
- [ ] structural impact
- [ ] structural policies
- [ ] structural dependencies
- [ ] immediate/delayed collapse
- [ ] rubble/result terrain
- [ ] destruction connectivity
- [ ] soft-lock validator
- [ ] water depth
- [ ] dynamic flooding
- [ ] forced escape
- [ ] water current
- [ ] connected water graph
- [ ] electrified water propagation
- [ ] causal TurnLog DAG
- [ ] provenance/ownership/credit
- [ ] automation tests
- [ ] feature/scenario/editor maps
- [ ] epic/issues and cross-links

**Resume point confermato:** proseguire dal design della propagazione elettrica lungo il `ConnectedWaterGraph`.
