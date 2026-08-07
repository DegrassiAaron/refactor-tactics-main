# REFACTORTACTICS — SISTEMA RUMORE, PERCEZIONE ACUSTICA E FOG OF WAR
## Specifica da usare come contesto per Claude

Sei uno specialista di game design tattico competitivo e Unreal Engine 5.
Stai lavorando su **RefactorTactics**, tattico competitivo PC-first in Unreal Engine 5 con turni simultanei, informazione incompleta, ambiente interattivo e simulazione deterministica.

Questa specifica definisce il sistema di **rumore e percezione acustica**.

---

# 1. OBIETTIVO DEL SISTEMA

Il rumore non deve essere trattato come un semplice debuff o come un raggio di rilevamento.

Deve essere una vera **risorsa informativa**, parallela alla visione.

Principio fondamentale:

> Non vedere un nemico non significa non poter sapere che si trova in una certa zona.

Il sistema deve creare decisioni tattiche relative a:

- velocità contro discrezione;
- movimento contro esposizione;
- uso dell'ambiente;
- depistaggio;
- coordinazione di squadra;
- Fast Reaction;
- controllo della Fog of War;
- deduzione delle azioni nemiche.

Il rumore deve essere leggibile, deterministico e utilizzabile strategicamente.

---

# 2. MODELLO DI INFORMAZIONE

La conoscenza di una squadra deve distinguere almeno:

## 2.1 Visibile

La squadra conosce:

- posizione esatta;
- identità;
- stato visibile;
- eventuali effetti pubblicamente osservabili.

Esempio:

`Visible Enemy: Unit_03 @ Cell H12`

---

## 2.2 Rilevato acusticamente

La squadra ha ricevuto un'informazione sonora ma non necessariamente conosce:

- posizione esatta;
- identità;
- abilità utilizzata;
- direzione precisa.

Possibili rappresentazioni:

- rumore a Nord-Est;
- rumore entro un'area di 5 celle;
- probabile rumore di movimento;
- probabile sparo;
- posizione quasi precisa.

---

## 2.3 Sconosciuto

Nessuna informazione disponibile alla squadra.

---

# 3. PRINCIPIO DI PRIVACY

La percezione acustica deve essere derivata esclusivamente da:

- stato pubblico;
- eventi realmente prodotti dalla simulazione;
- informazioni che la squadra è autorizzata a conoscere.

Non deve MAI permettere al client di ricevere:

- intenti nemici;
- percorsi pianificati nemici;
- target pianificati nemici;
- AoE pianificate;
- abilità pianificate;
- stato interno completo di unità non percepite.

Architettura concettuale:

```text
AUTHORITATIVE SIMULATION STATE
            |
            v
       PERCEPTION
            |
            v
      TEAM KNOWLEDGE
            |
            v
           UI
```

NON:

```text
Enemy Hidden State
        |
        v
      Enemy UI
```

Il server calcola la conoscenza disponibile per ciascuna squadra.

---

# 4. EVENTO DI RUMORE

Ogni azione rilevante può generare un evento sonoro.

Struttura concettuale:

```cpp
FRTNoiseEvent
{
    SourceId
    OriginCell
    NoiseType
    Intensity
    TurnIndex
    MicroStepIndex
}
```

Possibili campi aggiuntivi:

```text
SourceUnitId
SourceAbilityId
DurationMicroSteps
IsPersistent
IsDecoy
FrequencyClass
PropagationProfileId
```

Non aggiungere campi inutili finché non servono al vertical slice.

---

# 5. INTENSITÀ DEL RUMORE

Usare valori interi.

Esempio iniziale di bilanciamento:

| Azione | Noise |
|---|---:|
| Wait | 0 |
| Sneak | 0-1 |
| Walk | 1 |
| Move normale | 2 |
| Sprint | 5 |
| Dash | 6 |
| Apertura porta | 2 |
| Porta forzata | 7 |
| Attacco melee | 3 |
| Arma silenziata | 3 |
| Fucile | 7 |
| Esplosione | 10 |
| Crollo struttura | 10 |

Questi valori sono esclusivamente iniziali e devono essere data-driven.

---

# 6. MOVIMENTO E RUMORE

Il movimento deve introdurre una scelta tattica.

## Sneak

Vantaggi:

- rumore minimo;
- minor possibilità di localizzazione.

Svantaggi possibili:

- minore distanza;
- costo movimento maggiore;
- incompatibilità con alcune azioni;
- impossibilità di usare Sprint nello stesso planning.

---

## Move

Profilo standard.

---

## Sprint

Vantaggi:

- maggiore distanza;
- raggiungimento rapido di obiettivi;
- possibilità di fuga o pressione.

Svantaggi:

- forte rumore;
- possibili interazioni negative con terreno;
- maggiore prevedibilità.

Domanda tattica:

> Quanto lontano posso arrivare senza farmi localizzare?

---

# 7. PROPAGAZIONE DEL SUONO

Il rumore deve propagarsi sul **grafo tattico 3D** della mappa.

Non utilizzare un semplice `SphereOverlap` come autorità della simulazione.

Formula concettuale:

```text
RemainingNoise =
    SourceIntensity
    - DistanceCost
    - AcousticOcclusion
    - LocalAmbientMask
    + SurfacePropagationModifiers
```

Tutto deve usare:

- valori interi;
- ordinamento deterministico;
- regole data-driven;
- nessuna dipendenza dal frame rate.

---

# 8. TERRENI E MATERIALI

Ogni cella o transizione può modificare la propagazione sonora.

Valori iniziali indicativi:

| Terreno / superficie | Effetto |
|---|---:|
| Erba | -2 rumore movimento |
| Terra | -1 |
| Cemento | 0 |
| Metallo | +2 |
| Acqua | +2 |
| Ghiaccio | +1 |
| Ghiaccio + Sprint | +3 |
| Vetro | +1 |
| Macerie | +3 |
| Neve | -3 |
| Tunnel | riduce falloff |
| Porta aperta | lieve attenuazione |
| Porta chiusa | forte attenuazione |
| Parete | attenuazione elevata |

I valori reali dovranno essere configurabili.

---

# 9. TUNNEL ED ECO

I tunnel possono diventare elementi strategici specifici.

Esempio:

```text
Propagazione normale:
8 -> 7 -> 6 -> 5 -> 4

Tunnel:
8 -> 8 -> 7 -> 7 -> 6
```

Possibili proprietà:

```text
AcousticFalloffMultiplier
EchoBonus
Directionality
AmbientNoise
```

Il tunnel può rendere più semplice percepire un rumore ma più difficile determinarne l'origine esatta.

---

# 10. INTERAZIONE CON IL GHIACCIO

Il terreno ghiacciato genera rumore superiore durante movimento rapido.

Esempio:

```text
Walk on Ice:
Noise +1

Sprint on Ice:
Noise +3
Slip Risk / deterministic rule

Dash on Ice:
High Noise
Possible extended slide
```

Combo ambientale:

```text
ICE
  |
 FIRE
  v
WATER + STEAM
```

Conseguenze:

### Acqua

- passi più rumorosi;
- possibile conduzione elettrica.

### Vapore

- riduzione della visibilità;
- possibile copertura visiva.

Risultato:

> La zona diventa più difficile da vedere ma più facile da interpretare acusticamente.

Questo contrasto è desiderabile.

---

# 11. ATTRIBUTI ACUSTICI DEI PERSONAGGI

Separare almeno:

```text
HearingRange
HearingSensitivity
NoiseGeneration
NoiseSuppression
NoiseIdentification
```

Non usare necessariamente tutti questi valori nel primo vertical slice.

Possibile MVP:

```text
HearingThreshold
NoiseGenerationModifier
NoiseIdentificationLevel
```

---

# 12. ESEMPI DI ARCHETIPI

## Scout / Hunter

```text
HearingRange: alto
HearingSensitivity: alta
NoiseGeneration: basso
NoiseIdentification: alta
```

Può interpretare più precisamente:

- movimento;
- direzione;
- categoria della sorgente.

---

## Tank

```text
HearingRange: medio-basso
NoiseGeneration: molto alto
NoiseIdentification: bassa
```

È facile da localizzare, soprattutto durante Sprint e azioni pesanti.

Il rumore può però diventare intenzionalmente uno strumento di pressione o distrazione.

---

## Assassin

Caratteristiche:

- movimento silenzioso;
- forte NoiseSuppression;
- possibilità di usare rumore ambientale come copertura.

---

## Hacker

Possibili capacità:

- generare falsi rumori;
- attivare altoparlanti;
- far partire allarmi;
- imitare passi;
- manipolare dispositivi ambientali.

---

## Engineer

Può:

- spegnere generatori rumorosi;
- creare dispositivi acustici;
- modificare porte e pareti;
- controllare elementi ambientali.

---

# 13. PRECISIONE DELLA LOCALIZZAZIONE

Il rumore non deve rivelare automaticamente la cella esatta.

Usare livelli di precisione.

## Detection Level 1 — Direzione

Esempio:

`Rumore rilevato a Nord-Est`

---

## Detection Level 2 — Area larga

Esempio:

`Possibile sorgente entro 4-5 celle`

---

## Detection Level 3 — Area stretta

Esempio:

`Possibile sorgente entro 2 celle`

---

## Detection Level 4 — Posizione precisa

Esempio:

`Noise Source @ H12`

---

## Detection Level 5 — Identificazione

Esempio:

```text
Source: Steel
Action Category: Sprint
Cell: H12
```

Questo livello dovrebbe richiedere:

- personaggio specializzato;
- skill;
- distanza ridotta;
- rumore particolarmente riconoscibile.

---

# 14. NESSUN RNG NASCOSTO PER LA PERCEZIONE BASE

Evitare:

```text
65% chance to hear enemy
```

Preferire:

```text
ReceivedNoise >= HearingThreshold
```

Esempio:

```text
Source Noise = 6
Distance Loss = 2
Wall Attenuation = 2

ReceivedNoise = 2

Scout Threshold = 1
=> detected

Tank Threshold = 3
=> not detected
```

Il sistema deve poter essere compreso e previsto dal giocatore.

Eventuali elementi probabilistici futuri devono essere espliciti e usare seed deterministico, ma non sono necessari per il sistema base.

---

# 15. DECOY SONORI

Il rumore deve poter essere intenzionalmente falsificato.

## Noise Maker

Dispositivo:

```text
After 1 micro-step:
Generate Noise 8 at target cell
```

---

## Phantom Steps

Genera una sequenza:

```text
A4 -> B4 -> C5
```

come se un'unità si stesse muovendo.

Il nemico rileva un evento vero:

> è realmente esistito un rumore.

Ma la sua interpretazione può essere errata.

Questo mantiene coerenza con la simulazione.

---

# 16. FAST REACTION E RUMORE

Il sistema di Fast Reaction può usare eventi acustici come trigger.

Esempio:

## Ambush Reaction

Trigger:

```text
Enemy Noise >= 5
within 3 cells
```

Durante Resolution:

```text
FOOTSTEPS DETECTED

Possible enemy approaching

[OVERWATCH]
[TAKE COVER]
```

Il giocatore può reagire senza necessariamente vedere il nemico.

Questo introduce decisioni basate su informazione incompleta.

---

# 17. ABILITÀ BASATE SUL SUONO

Esempi da usare per brainstorming e prototipi.

## Sonar Pulse

Rileva fonti sonore recenti entro un certo numero di celle.

---

## Hunter's Ear

Se un nemico genera rumore sopra una soglia:

```text
gain targeting bonus / improved localization
```

per il resto del turno.

---

## Silent Step

```text
MovementNoise = 0
for N cells
```

oppure:

```text
MovementNoiseModifier = -X
```

Preferire la seconda se più bilanciabile.

---

## Resonance Shot

Permette di attaccare una sorgente localizzata acusticamente ma non visibile.

Possibili penalità:

```text
AccuracyPenalty
DamagePenalty
MaximumUncertaintyRadius
```

---

## Sonic Grenade

Effetto possibile:

```text
Deafened
HearingRange reduction
HearingThreshold increase
Fast Reaction acoustic triggers disabled
```

Può avere danno nullo o molto ridotto.

---

# 18. MASCHERAMENTO ACUSTICO

Una sorgente molto rumorosa può nascondere sorgenti più deboli.

Esempio:

```text
Explosion:
Noise 10

Creates:
AcousticMask 8
Duration 2 microsteps
```

Durante tale intervallo:

```text
Assassin Sprint
Noise 5
```

può non essere percepibile in quella zona.

Questo permette combo di squadra.

Esempio:

```text
Tank detonates grenade
        +
Assassin crosses exposed area
```

Il rumore diventa una forma di "smoke grenade acustica".

---

# 19. RUMORE AMBIENTALE

La mappa può produrre rumore indipendente dai personaggi.

Esempi:

```text
Fire
Generator
Waterfall
Alarm
Rain
Ventilation
Industrial machinery
Electrical arcs
Collapsing structures
```

Esempio:

```text
Waterfall
AmbientNoise = 5
```

La cascata può creare una zona in cui:

- movimento leggero è difficile da percepire;
- esplosioni restano percepibili;
- identificazione della sorgente peggiora.

Questo crea vere zone di **copertura acustica**.

---

# 20. ACQUA + ELETTRICITÀ

La combo ambientale:

```text
WATER
 +
ELECTRICITY
 =
ELECTRIFIED WATER
```

può produrre rumore:

```text
NoiseType = Noise.Environment.Electric
Intensity = 6
```

Un giocatore potrebbe dedurre:

> Qualcosa ha elettrificato l'acqua in quella zona.

Il rumore quindi non comunica soltanto una posizione, ma permette deduzione strategica sugli eventi.

---

# 21. MEMORIA SONORA

Una squadra può mantenere traccia degli eventi uditi.

Esempio dati:

```text
LastHeardTurn
LastHeardMicroStep
LastHeardArea
LastNoiseType
Confidence
```

UI:

```text
Current turn:
strong marker

Previous turn:
faded marker

Two turns ago:
very faded / removed
```

Questa informazione è "ghost intel", non posizione attuale.

Non deve aggiornarsi segretamente se la sorgente non viene più percepita.

---

# 22. GAMEPLAY TAGS

Possibile tassonomia:

```text
Noise.Movement
Noise.Movement.Walk
Noise.Movement.Sprint
Noise.Movement.Dash

Noise.Weapon
Noise.Weapon.Pistol
Noise.Weapon.Rifle
Noise.Weapon.Melee

Noise.Explosion

Noise.Environment
Noise.Environment.Fire
Noise.Environment.Water
Noise.Environment.Electric
Noise.Environment.Mechanical

Noise.Voice

Noise.Decoy
```

Usare Gameplay Tags governati.

Le abilità possono filtrare per categoria.

Esempio:

```text
Detect:
Noise.Movement.*

Ignore:
Noise.Environment.*
```

---

# 23. UI E SOUND OVERLAY

Il giocatore deve poter capire il sistema senza una valanga di numeri.

Possibili overlay:

```text
Vision
Movement
Threat
Sound
Terrain
```

La modalità `Sound` può mostrare:

- sorgenti udite;
- area di incertezza;
- direzione probabile;
- intensità;
- età dell'informazione;
- rumore ambientale;
- copertura acustica.

Durante il planning può mostrare anche:

> propagazione stimata del rumore prodotto dalle PROPRIE azioni.

Non deve utilizzare informazioni segrete nemiche.

---

# 24. STATI UI

Integrare con la filosofia:

- **Confermato**
- **Previsto**
- **Incerto**

Esempi:

## Confermato

```text
Loud explosion heard at Cell H12.
```

## Previsto

```text
Your Sprint will likely be audible in sector B.
```

Il calcolo usa esclusivamente informazioni consentite.

## Incerto

```text
Movement noise detected somewhere in this area.
```

---

# 25. TEAM KNOWLEDGE

Il risultato della percezione sonora deve essere inserito in un modello di conoscenza di squadra.

Concettualmente:

```text
Simulation
    |
Noise Events
    |
Sound Propagation
    |
Unit Perception
    |
Team Perception Merge
    |
Team Knowledge
    |
Replication / Client RPC
    |
UI
```

Il server è autorità assoluta.

Se due alleati percepiscono la stessa sorgente con precisione diversa, il Team Knowledge può utilizzare l'informazione migliore disponibile.

---

# 26. NETWORKING

Non replicare gli eventi di rumore globalmente a tutti.

Possibile flusso:

```text
Server authoritative simulation
        |
        v
Generate FRTNoiseEvent
        |
        v
Calculate perception by team
        |
        v
Create sanitized team intel
        |
        v
Send/replicate only allowed information
```

Il client nemico non deve ricevere l'evento completo se non lo può percepire.

Dati sanitizzati possibili:

```text
NoiseTypeCategory
ApproximateArea
Direction
Confidence
Turn
MicroStep
IntensityClass
```

NON necessariamente:

```text
SourceUnitId
ExactOriginCell
AbilityId
```

a meno che la percezione lo consenta.

---

# 27. ARCHITETTURA UNREAL PROPOSTA

Possibili sistemi:

```text
URTNoisePropagationSubsystem
URTPerceptionSubsystem
URTTeamKnowledgeSubsystem
```

oppure servizi posseduti dal simulatore.

Per il vertical slice preferire la soluzione più semplice che:

- non accoppi UI e simulazione;
- sia testabile;
- non replichi informazioni proibite.

Strutture suggerite:

```cpp
FRTNoiseEvent
FRTNoisePropagationResult
FRTPerceivedNoise
FRTTeamNoiseIntel
```

Non creare Actor per ogni rumore.

Il rumore deve essere dato logico temporaneo.

---

# 28. INTEGRAZIONE CON IL GRAFO TATTICO

Il servizio di propagazione deve utilizzare lo stesso grafo tattico delle celle ma NON il pathfinding come semantica.

Pathfinding e propagazione sonora sono due query differenti sullo stesso grafo.

Esempio:

```text
Tactical Graph
   |
   +-- Movement Query
   +-- LOS Query
   +-- Targeting Query
   +-- Sound Propagation Query
```

Ogni transizione può avere:

```text
MovementCost
SoundAttenuation
VisibilityOcclusion
ProjectileRules
```

separati.

---

# 29. DETERMINISMO

Vincoli:

- niente dipendenza dal frame rate;
- niente ordine basato su `TMap` non ordinata;
- ID stabili;
- valori interi/fixed-point;
- iterazioni ordinate;
- stesso snapshot + stessi eventi + stesse regole = stesso risultato.

L'eventuale propagazione sonora deve produrre risultati identici in replay.

---

# 30. TURN LOG

Gli eventi sonori rilevanti devono essere registrabili.

Esempio log server:

```text
Turn 4 / MicroStep 3
NoiseEvent:
Source=Unit03
Origin=H12
Type=Noise.Movement.Sprint
Intensity=5

TeamA:
Detected=true
Precision=Area2

TeamB:
Detected=false
```

Il log di debug completo deve restare server/dev.

Il log giocatore mostra soltanto le informazioni consentite.

---

# 31. POSSIBILE ALGORITMO DI PROPAGAZIONE

Per il primo prototipo è sufficiente una ricerca tipo Dijkstra/flood fill limitata dall'intensità.

Pseudo:

```text
OpenSet = Origin
Cost[Origin] = 0

while OpenSet not empty:

    Node = lowest acoustic cost

    for Neighbor in Node:

        NewCost =
            Cost[Node]
            + DistanceCost
            + EdgeSoundAttenuation
            + CellSoundAttenuation

        if NewCost < SourceIntensity:
            visit Neighbor
```

Per ogni cella:

```text
ReceivedNoise = SourceIntensity - AcousticCost
```

Successivamente applicare:

```text
AmbientMask
UnitHearingThreshold
IdentificationRules
```

Evitare premature ottimizzazioni.

---

# 32. PERFORMANCE

Il rumore deve restare compatibile con gli obiettivi del simulatore.

Possibili ottimizzazioni future:

- limite massimo di propagazione;
- early out;
- bucket integer priority queue;
- caching di attenuation statica;
- invalidazione tramite revision del chunk;
- propagazione aggregata di sorgenti ambientali persistenti;
- precompute acustico per porte/stanze se necessario.

Non implementare ottimizzazioni complesse prima di profilare.

---

# 33. BILANCIAMENTO DATA-DRIVEN

Configurare da Data Asset / catalogo:

```text
Noise intensity per action
Surface modifiers
Door attenuation
Wall attenuation
Tunnel modifiers
Ambient noise
Hearing thresholds
Character modifiers
Ability modifiers
Decay
Memory duration
Localization thresholds
```

Niente valori gameplay hard-coded sparsi nel codice.

---

# 34. VERTICAL SLICE MINIMO

Per il primo test implementare soltanto:

1. `FRTNoiseEvent`;
2. rumore generato da Move e Sprint;
3. propagazione su griglia;
4. parete che attenua;
5. acqua che aumenta rumore movimento;
6. un'unità con HearingThreshold standard;
7. una con hearing migliore;
8. indicatore UI di area rumorosa;
9. TurnLog;
10. Automation Test deterministico.

NON implementare subito:

- sonar complesso;
- eco avanzato;
- frequency simulation;
- audio fisico;
- decoy multipli;
- AI hearing UE come autorità;
- networking completo delle abilità sonore.

---

# 35. TEST MINIMI

## Test 1 — Distance

```text
Noise = 5
distance = 3
=> ReceivedNoise = expected value
```

---

## Test 2 — Wall

Con parete tra sorgente e ascoltatore:

```text
ReceivedNoiseWithWall < ReceivedNoiseWithoutWall
```

---

## Test 3 — Character Hearing

```text
ReceivedNoise = 2

ScoutThreshold = 1
TankThreshold = 3

Scout => hears
Tank => does not hear
```

---

## Test 4 — Determinism

Eseguire la stessa query 100 volte.

Il risultato deve essere identico.

---

## Test 5 — Privacy

Un team che non percepisce l'evento non deve ricevere:

```text
OriginCell
SourceUnitId
AbilityId
```

---

## Test 6 — Sound Masking

```text
AmbientNoise = 5
MovementNoiseReceived = 3
=> not detected
```

se questa è la regola configurata.

---

# 36. DEBUG

Creare debug overlay sviluppatore.

Esempio:

```text
Cell H12
AcousticCost = 0
Received = 8

Cell H13
AcousticCost = 1
Received = 7

Cell H14
AcousticCost = 3
Received = 5
```

Possibile visualizzazione:

```text
8 7 6 5
7 6 4 3
5 4 # 1
```

dove `#` indica ostacolo.

Debug disponibile solo in development/editor.

---

# 37. DESIGN PRINCIPLE FINALE

Il rumore deve essere pensato come:

> una seconda rete informativa della partita.

Schema:

```text
                  INFORMATION
                       |
             +---------+---------+
             |                   |
           VISION               SOUND
             |                   |
         LOS / Cover         Propagation
        Smoke / Steam      Walls / Terrain
          Stealth          Ambient Noise
             |                   |
             +---------+---------+
                       |
                 TEAM KNOWLEDGE
```

Il giocatore deve poter:

- fare rumore per muoversi più velocemente;
- evitarlo per infiltrarsi;
- ascoltarlo per localizzare nemici;
- coprirlo con esplosioni;
- sfruttare una cascata o macchinari;
- generarlo artificialmente;
- dedurre cosa è successo;
- usare Fast Reaction;
- coordinarsi con gli alleati;
- sfruttare terreno e abilità.

Il rumore non è solo penalità.

È:

> **informazione, rischio, depistaggio e controllo del territorio.**

---

# 38. RICHIESTA A CLAUDE

Quando lavori su questa feature:

1. mantieni la compatibilità con l'architettura di RefactorTactics;
2. considera Unreal Engine 5 stabile bloccata per milestone;
3. usa C++ per simulazione, rete e validazione;
4. usa Blueprint/UI soltanto come presentazione/configurazione;
5. non usare `UAIPerception` come autorità della simulazione competitiva;
6. usa il tactical graph centralizzato;
7. usa valori interi;
8. garantisci determinismo;
9. non replicare informazioni nascoste;
10. crea codice compilabile e testabile;
11. implementa prima il vertical slice minimo;
12. segnala chiaramente API UE specifiche di versione;
13. includi Automation Tests;
14. includi debug visualization;
15. mantieni il sistema data-driven.

Per ogni proposta tecnica indica:

- assunzioni;
- file da creare/modificare;
- dipendenze;
- codice C++ essenziale completo;
- setup Editor;
- procedura di compilazione;
- test manuali;
- Automation Tests;
- errori comuni;
- debug;
- commit Git suggerito;
- passo successivo.

Non ampliare lo scope senza necessità.
